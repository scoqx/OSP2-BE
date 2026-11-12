/*
===========================================================================
Glue between OSP2-BE and the Quake Live movement in this directory

ql_bg_pmove.c / ql_bg_slidemove.c are qalive's own movement code. This file
gives them what they need from QL's bg_misc.c and runs them on an OSP2-BE
(Quake III) playerState:

  Q3 playerState  -> QL playerState   inverse of the server's packing
  QL Pmove
  QL playerState  -> Q3 playerState   the server's packing (qalive
                                      sv_q3compat.c SV_Q3_TranslatePlayerstate)

so the predicted state comes out exactly as the next snapshot will.
===========================================================================
*/

#include "ql_shared.h"
#include "bg_public.h"
#include "bg_local.h"
#include "q3wire_tables.h"
#include "../bg_pmove_ql_io.h"

/*
=============================================================================

What QL's bg_misc.c gives the movement

=============================================================================
*/

// weapon fire intervals, from the server's CS_WEAPONINFO (defaults: qalive bg_misc.c)
int bg_weaponReloadTime[WP_NUM_WEAPONS] = {
    0,    // WP_NONE
    400,  // WP_GAUNTLET
    100,  // WP_MACHINEGUN
    1000, // WP_SHOTGUN
    800,  // WP_GRENADE_LAUNCHER
    800,  // WP_ROCKET_LAUNCHER
    50,   // WP_LIGHTNING
    1500, // WP_RAILGUN
    100,  // WP_PLASMAGUN
    300,  // WP_BFG
    100,  // WP_GRAPPLING_HOOK
    1000, // WP_NAILGUN
    800,  // WP_PROX_LAUNCHER
    50,   // WP_CHAINGUN
    75,   // WP_HMG
};

/*
The movement only reads items for the held item: bg_itemlist[stats[STAT_HOLDABLE_ITEM]].
That stat arrives as an OSP2-BE item index (the server translates it), so this
list is indexed like OSP2-BE's bg_itemlist and only knows the holdables, which
OSP2-BE registers through QLPM_SetHoldableItem. Q3 and QL number holdables the
same (teleporter 1 .. invulnerability 5).
*/
#define QLPM_MAX_ITEMS 256
gitem_t bg_itemlist[QLPM_MAX_ITEMS];
int bg_numItems = QLPM_MAX_ITEMS;

void QLPM_SetHoldableItem(int itemNum, int holdableTag) {
    if (itemNum <= 0 || itemNum >= QLPM_MAX_ITEMS) {
        return;
    }
    bg_itemlist[itemNum].giType = IT_HOLDABLE;
    bg_itemlist[itemNum].giTag = holdableTag;
}

gitem_t* BG_FindItemForHoldable(holdable_t pw) {
    int i;

    for (i = 1; i < QLPM_MAX_ITEMS; i++) {
        if (bg_itemlist[i].giType == IT_HOLDABLE && bg_itemlist[i].giTag == (int)pw) {
            return &bg_itemlist[i];
        }
    }
    return NULL;
}

holdable_t BG_HoldableForItemTag(int itemTag) {
    if (itemTag <= HI_NONE || itemTag >= HI_NUM_HOLDABLE) {
        return HI_NONE;
    }
    return (holdable_t)itemTag;
}

// QL keeps runes in STAT_RUNE, which the server does not send to Q3 clients
qboolean BG_PlayerHasPersistantPowerup(const playerState_t* ps, powerup_t powerup) {
    (void)ps;
    (void)powerup;
    return qfalse;
}

void BG_AddPredictableEventToPlayerstate(int newEvent, int eventParm, playerState_t* ps) {
    ps->events[ps->eventSequence & (MAX_PS_EVENTS - 1)] = newEvent;
    ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS - 1)] = eventParm;
    ps->eventSequence++;
}

/*
=============================================================================

Server settings (configstrings)

=============================================================================
*/

typedef struct {
    const char* key;
    void (*setFloat)(float value);
    void (*setInt)(int value);
} qlpmSetting_t;

// qalive g_main.c G_SendPmoveInfo keys
static const qlpmSetting_t qlpmSettings[] = {
    {"pmove_AirAccel", PM_SetAirAccel, NULL},
    {"pmove_AirStepFriction", PM_SetAirStepFriction, NULL},
    {"pmove_AirSteps", NULL, PM_SetAirSteps},
    {"pmove_AirStopAccel", PM_SetAirStopAccel, NULL},
    {"pmove_AirControl", PM_SetAirControl, NULL},
    {"pmove_AutoHop", NULL, PM_SetAutoHop},
    {"pmove_BunnyHop", NULL, PM_SetBunnyHop},
    {"pmove_ChainJump", NULL, PM_SetChainJump},
    {"pmove_ChainJumpVelocity", PM_SetChainJumpVelocity, NULL},
    {"pmove_CircleStrafeFriction", PM_SetCircleStrafeFriction, NULL},
    {"pmove_CrouchSlideFriction", PM_SetCrouchSlideFriction, NULL},
    {"pmove_CrouchSlideTime", NULL, PM_SetCrouchSlideTime},
    {"pmove_CrouchSlide", NULL, PM_SetCrouchSlide},
    {"pmove_CrouchStepJump", NULL, PM_SetCrouchStepJump},
    {"pmove_DoubleJump", NULL, PM_SetDoubleJump},
    {"pmove_JumpTimeDeltaMin", PM_SetJumpTimeDeltaMin, NULL},
    {"pmove_JumpVelocity", PM_SetJumpVelocity, NULL},
    {"pmove_JumpVelocityMax", PM_SetJumpVelocityMax, NULL},
    {"pmove_JumpVelocityScaleAdd", PM_SetJumpVelocityScaleAdd, NULL},
    {"pmove_JumpVelocityTimeThreshold", PM_SetJumpVelocityTimeThreshold, NULL},
    {"pmove_JumpVelocityTimeThresholdOffset", PM_SetJumpVelocityTimeThresholdOffset, NULL},
    {"pmove_noPlayerClip", NULL, PM_SetNoPlayerClip},
    {"pmove_RampJump", NULL, PM_SetRampJump},
    {"pmove_RampJumpScale", PM_SetRampJumpScale, NULL},
    {"pmove_StepHeight", PM_SetStepHeight, NULL},
    {"pmove_StepJump", NULL, PM_SetStepJump},
    {"pmove_StepJumpVelocity", PM_SetStepJumpVelocity, NULL},
    {"pmove_StrafeAccel", PM_SetStrafeAccel, NULL},
    {"pmove_velocity_gh", PM_SetVelocityGH, NULL},
    {"pmove_WalkAccel", PM_SetWalkAccel, NULL},
    {"pmove_WalkFriction", PM_SetWalkFriction, NULL},
    {"pmove_WaterSwimScale", PM_SetWaterSwimScale, NULL},
    {"pmove_WaterWadeScale", PM_SetWaterWadeScale, NULL},
    {"pmove_WeaponRaiseTime", NULL, PM_SetWeaponRaiseTime},
    {"pmove_WeaponDropTime", NULL, PM_SetWeaponDropTime},
    {"pmove_WishSpeed", PM_SetWishSpeed, NULL},
};

void QLPM_ParsePmoveInfo(const char* info) {
    int i;

    if (!info || !info[0]) {
        return;
    }
    for (i = 0; i < (int)(sizeof(qlpmSettings) / sizeof(qlpmSettings[0])); i++) {
        const char* val = Info_ValueForKey(info, qlpmSettings[i].key);

        if (!val[0]) {
            continue;
        }
        if (qlpmSettings[i].setFloat) {
            qlpmSettings[i].setFloat(atof(val));
        } else {
            qlpmSettings[i].setInt(atoi(val));
        }
    }
}

// the server's CS_WEAPONINFO: weapon_reload_* fire intervals (qalive BG_SetWeaponReload)
void QLPM_ParseWeaponInfo(const char* info) {
    static const struct {
        const char* key;
        int weapon;
    } reloads[] = {
        {"weapon_reload_gauntlet", WP_GAUNTLET},
        {"weapon_reload_mg", WP_MACHINEGUN},
        {"weapon_reload_sg", WP_SHOTGUN},
        {"weapon_reload_gl", WP_GRENADE_LAUNCHER},
        {"weapon_reload_rl", WP_ROCKET_LAUNCHER},
        {"weapon_reload_lg", WP_LIGHTNING},
        {"weapon_reload_rg", WP_RAILGUN},
        {"weapon_reload_pg", WP_PLASMAGUN},
        {"weapon_reload_bfg", WP_BFG},
        {"weapon_reload_gh", WP_GRAPPLING_HOOK},
        {"weapon_reload_ng", WP_NAILGUN},
        {"weapon_reload_prox", WP_PROX_LAUNCHER},
        {"weapon_reload_cg", WP_CHAINGUN},
        {"weapon_reload_hmg", WP_HMG},
    };
    int i;

    if (!info || !info[0]) {
        return;
    }
    for (i = 0; i < (int)(sizeof(reloads) / sizeof(reloads[0])); i++) {
        const char* val = Info_ValueForKey(info, reloads[i].key);

        if (val[0]) {
            bg_weaponReloadTime[reloads[i].weapon] = atoi(val);
        }
    }
}

/*
=============================================================================

Q3 <-> QL playerState

=============================================================================
*/

// QL value -> Q3 value, or Q3W_DROP
static int QLPM_Map(const int* table, int count, int value) {
    if (value < 0 || value >= count) {
        return Q3W_DROP;
    }
    return table[value];
}

// HMG shows as the machinegun; nailgun / prox / chaingun have no OSP2-BE slot
static int QLPM_Q3Weapon(int qlWeapon) {
    switch (qlWeapon) {
        case QLW_WP_HMG:
            return QLW_WP_MACHINEGUN;
        case QLW_WP_NAILGUN:
        case QLW_WP_PROX_LAUNCHER:
        case QLW_WP_CHAINGUN:
            return WP_NONE;
        default:
            return qlWeapon;
    }
}

static int QLPM_Q3PmFlags(int qlFlags) {
    int i, flags = 0;

    for (i = 0; i < (int)Q3W_NUM_PMFLAGS; i++) {
        if (qlFlags & q3w_pmflag[i][0]) {
            flags |= q3w_pmflag[i][1];
        }
    }
    return flags;
}

static int QLPM_QLPmFlags(int q3Flags) {
    int i, flags = 0;

    for (i = 0; i < (int)Q3W_NUM_PMFLAGS; i++) {
        if (q3Flags & q3w_pmflag[i][1]) {
            flags |= q3w_pmflag[i][0];
        }
    }
    return flags;
}

// an event the movement just added: QL number -> OSP2-BE number (it adds no item / obituary events)
static int QLPM_Q3Event(int qlEvent) {
    int ev = QLPM_Map(q3w_event, Q3W_NUM_QL_EVENTS, qlEvent & ~EV_EVENT_BITS);

    return (ev < 0 ? Q3W_EV_NONE : ev) | (qlEvent & EV_EVENT_BITS);
}

/*
==================
QLPM_FromQ3

Inverse of the server's packing for everything the movement reads. Fields Q3
does not carry stay 0, like the server's own values for a Q3 client.
==================
*/
static void QLPM_FromQ3(const q3playerState_t* in, playerState_t* out) {
    int i;

    memset(out, 0, sizeof(*out));

    out->commandTime = in->commandTime;
    out->pm_type = in->pm_type;
    out->bobCycle = in->bobCycle;
    out->pm_flags = QLPM_QLPmFlags(in->pm_flags) | (in->pm_flags & QLPM_PMF_NO_AUTOHOP);
    out->pm_time = in->pm_time;
    VectorCopy(in->origin, out->origin);
    VectorCopy(in->velocity, out->velocity);
    out->weaponTime = in->weaponTime;
    out->gravity = in->gravity;
    out->speed = in->speed;
    for (i = 0; i < 3; i++) {
        out->delta_angles[i] = in->delta_angles[i];
    }
    out->groundEntityNum = in->groundEntityNum;
    out->legsTimer = in->legsTimer;
    out->legsAnim = in->legsAnim;
    out->torsoTimer = in->torsoTimer;
    out->torsoAnim = in->torsoAnim;
    out->movementDir = in->movementDir;
    VectorCopy(in->grapplePoint, out->grapplePoint);
    out->eFlags = in->eFlags;
    out->eventSequence = in->eventSequence;
    out->clientNum = in->clientNum;
    out->weapon = in->weapon;
    out->weaponstate = in->weaponstate;
    VectorCopy(in->viewangles, out->viewangles);
    out->viewheight = in->viewheight;

    for (i = 0; i < Q3W_NUM_STATS; i++) {
        if (q3w_stat[i] >= 0) {
            out->stats[i] = in->stats[q3w_stat[i]];
        }
    }
    for (i = 0; i < Q3W_NUM_PERSISTANT; i++) {
        if (q3w_pers[i] >= 0) {
            out->persistant[i] = in->persistant[q3w_pers[i]];
        }
    }
    for (i = 0; i < Q3W_NUM_POWERUPS; i++) {
        if (q3w_powerup[i] > 0) {
            out->powerups[i] = in->powerups[q3w_powerup[i]];
        }
    }
    for (i = 0; i < MAX_WEAPONS; i++) {
        out->ammo[i] = in->ammo[i];
    }

    out->generic1 = in->generic1;
    out->loopSound = in->loopSound;
    out->jumppad_ent = in->jumppad_ent;

    // QL-only movement state, see q3wire_tables.h
    out->jumpTime = in->commandTime - in->stats[Q3W_STAT_JUMPAGE];
    out->crouchSlideTime = in->stats[Q3W_STAT_CROUCHSLIDE];
    out->doubleJumped = (in->pm_flags & Q3W_PMF_DOUBLE_JUMPED_STATE) ? qtrue : qfalse;

    out->ping = in->ping;
    out->pmove_framecount = in->pmove_framecount;
    out->jumppad_frame = in->jumppad_frame;
    out->entityEventSequence = in->entityEventSequence;
}

/*
==================
QLPM_ToQ3

The server's packing (SV_Q3_TranslatePlayerstate), written over the Q3 state
the movement started from: what the movement does not touch stays as received.
==================
*/
static void QLPM_ToQ3(const playerState_t* in, q3playerState_t* out, int oldEventSequence) {
    int i, seq, age;

    out->commandTime = in->commandTime;
    out->pm_type = in->pm_type;
    out->bobCycle = in->bobCycle;
    out->pm_flags = QLPM_Q3PmFlags(in->pm_flags) | (in->pm_flags & QLPM_PMF_NO_AUTOHOP);
    if (in->doubleJumped) {
        out->pm_flags |= Q3W_PMF_DOUBLE_JUMPED_STATE;
    }
    out->pm_time = in->pm_time;
    VectorCopy(in->origin, out->origin);
    VectorCopy(in->velocity, out->velocity);
    out->weaponTime = in->weaponTime;
    out->gravity = in->gravity;
    out->speed = in->speed;
    for (i = 0; i < 3; i++) {
        out->delta_angles[i] = in->delta_angles[i];
    }
    out->groundEntityNum = in->groundEntityNum;
    out->legsTimer = in->legsTimer;
    out->legsAnim = in->legsAnim;
    out->torsoTimer = in->torsoTimer;
    out->torsoAnim = in->torsoAnim;
    out->movementDir = in->movementDir;
    VectorCopy(in->grapplePoint, out->grapplePoint);
    out->eFlags = in->eFlags;

    // only the events this move added are new; older ones are already in Q3 numbers
    for (seq = oldEventSequence; seq < in->eventSequence; seq++) {
        int slot = seq & (MAX_PS_EVENTS - 1);

        out->events[slot] = QLPM_Q3Event(in->events[slot]);
        out->eventParms[slot] = in->eventParms[slot];
    }
    out->eventSequence = in->eventSequence;

    out->weapon = QLPM_Q3Weapon(in->weapon);
    out->weaponstate = in->weaponstate;
    VectorCopy(in->viewangles, out->viewangles);
    out->viewheight = in->viewheight;

    for (i = 0; i < Q3W_NUM_STATS; i++) {
        if (q3w_stat[i] >= 0) {
            out->stats[q3w_stat[i]] = in->stats[i];
        }
    }
    age = in->commandTime - in->jumpTime;
    out->stats[Q3W_STAT_JUMPAGE] = age < 0 ? 0 : (age > Q3W_AGE_MAX ? Q3W_AGE_MAX : age);
    out->stats[Q3W_STAT_CROUCHSLIDE] = in->crouchSlideTime;

    for (i = 0; i < MAX_WEAPONS; i++) {
        out->ammo[i] = in->ammo[i];
    }
    out->ammo[QLW_WP_HMG] = 0;

    out->generic1 = in->generic1;
    out->loopSound = in->loopSound;
    out->jumppad_ent = in->jumppad_ent;
    out->pmove_framecount = in->pmove_framecount;
    out->jumppad_frame = in->jumppad_frame;
}

/*
=============================================================================

Running it

=============================================================================
*/

float qlpm_stepHeight;
int qlpm_stepTime;

void QLPM_Run(qlpmIo_t* io) {
    q3playerState_t* q3ps = (q3playerState_t*)io->ps;
    playerState_t ps;
    pmove_t move;
    int oldEventSequence;

    QLPM_FromQ3(q3ps, &ps);
    oldEventSequence = ps.eventSequence;

    memset(&move, 0, sizeof(move));
    move.ps = &ps;
    // the server reads a Q3 usercmd the same way (qalive MSG_ReadDeltaUsercmdKeyQ3)
    move.cmd.serverTime = io->serverTime;
    move.cmd.angles[0] = io->angles[0];
    move.cmd.angles[1] = io->angles[1];
    move.cmd.angles[2] = io->angles[2];
    move.cmd.buttons = io->buttons;
    move.cmd.weapon = (byte)io->weapon;
    move.cmd.weaponPrimary = 0;
    move.cmd.fov = 0;
    move.cmd.forwardmove = (signed char)io->forwardmove;
    move.cmd.rightmove = (signed char)io->rightmove;
    move.cmd.upmove = (signed char)io->upmove;
    move.tracemask = io->tracemask;
    move.debugLevel = io->debugLevel;
    move.noFootsteps = io->noFootsteps;
    move.gauntletHit = io->gauntletHit;
    move.trace = io->trace;
    move.pointcontents = io->pointcontents;

    Pmove(&move);

    QLPM_ToQ3(&ps, q3ps, oldEventSequence);

    io->numtouch = move.numtouch;
    memcpy(io->touchents, move.touchents, sizeof(io->touchents));
    VectorCopy(move.mins, io->mins);
    VectorCopy(move.maxs, io->maxs);
    io->watertype = move.watertype;
    io->waterlevel = move.waterlevel;
    io->xyspeed = move.xyspeed;

    // keep the last stair step for the view smoothing (cg_predict.c CG_QLStepSmooth)
    if (move.stepTime) {
        qlpm_stepHeight = move.stepHeight;
        qlpm_stepTime = move.stepTime;
    }
}
