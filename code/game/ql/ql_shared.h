/*
===========================================================================
Quake Live types for the QL movement in this directory (OSP2-BE)

The files next to this one are qalive's bg_pmove.c / bg_slidemove.c and their
headers, copied by tools/sync_ql_pmove.sh. They are built against OSP2-BE's
q_shared.h with two types swapped for their Quake Live layout:

  playerState_t, usercmd_t  QL versions; the Q3 ones stay as q3playerState_t /
                            q3usercmd_t for the glue in ql_glue.c

ql_rename.h gives every global symbol of these files a qlpm_ prefix so they
can sit in the same QVM as OSP2-BE's own pmove.
===========================================================================
*/
#ifndef QL_SHARED_H
#define QL_SHARED_H

#include "ql_rename.h"

#ifdef __GNUC__
// q_shared.h's MSVC #pragma warning lines; qalive's code as is
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

#define playerState_s q3playerState_s
#define playerState_t q3playerState_t
#define usercmd_s q3usercmd_s
#define usercmd_t q3usercmd_t
#include "../../qcommon/q_shared.h"
#undef playerState_s
#undef playerState_t
#undef usercmd_s
#undef usercmd_t

// QL surface flags missing from Q3's surfaceflags.h (qalive surfaceflags.h)
#ifndef SURF_SNOWSTEPS
#define SURF_SNOWSTEPS 0x80000
#endif
#ifndef SURF_WOODSTEPS
#define SURF_WOODSTEPS 0x100000
#endif

// q3lcc has no long double: QL keeps some products in x87 precision, the QVM
// computes them in float (last-bit differences only)
#ifdef Q3_VM
#define sqrtl sqrt
#endif

// from qalive code/qcommon/q_shared.h
typedef struct playerState_s {
    int commandTime;          // +0   cmd->serverTime of last executed command
    int pm_type;              // +4
    int bobCycle;             // +8   for view bobbing and footstep generation
    int pm_flags;             // +12  ducked, jump_held, etc
    int pm_time;              // +16

    vec3_t origin;            // +20
    vec3_t velocity;          // +32
    int weaponTime;           // +44
    int gravity;              // +48
    int speed;                // +52
    int delta_angles[3];      // +56  add to command angles to get view direction

    int groundEntityNum;      // +68  ENTITYNUM_NONE = in air

    int legsTimer;            // +72
    int legsAnim;             // +76

    int torsoTimer;           // +80
    int torsoAnim;            // +84

    int movementDir;          // +88

    vec3_t grapplePoint;      // +92  location of grapple to pull towards

    int eFlags;               // +104 copied to entityState_t->eFlags

    int eventSequence;        // +108 pmove generated events
    int events[MAX_PS_EVENTS];    // +112
    int eventParms[MAX_PS_EVENTS]; // +120

    int externalEvent;        // +128 events set on player from another source
    int externalEventParm;    // +132
    // [QL] removed: externalEventTime (was here in Q3)

    int clientNum;            // +136 ranges from 0 to MAX_CLIENTS-1
    int location;             // +140 [QL]
    int weapon;               // +144 copied to entityState_t->weapon
    int weaponPrimary;        // +148 [QL]
    int weaponstate;          // +152

    int fov;                  // +156 [QL]
    vec3_t viewangles;        // +160 for fixed views
    int viewheight;           // +172

    // damage feedback
    int damageEvent;          // +176 when it changes, latch the other parms
    int damageYaw;            // +180
    int damagePitch;          // +184
    int damageCount;          // +188

    int stats[MAX_STATS];             // +192
    int persistant[MAX_PERSISTANT];   // +256 stats that aren't cleared on death
    int powerups[MAX_POWERUPS];       // +320 level.time that the powerup runs out
    int ammo[MAX_WEAPONS];            // +384

    int generic1;             // +448
    int loopSound;            // +452
    int jumppad_ent;          // +456 jumppad entity hit this frame

    int jumpTime;             // +460 [QL]
    int doubleJumped;         // +464 [QL]
    int crouchTime;           // +468 [QL]
    int crouchSlideTime;      // +472 [QL]
    char forwardmove;         // +476 [QL]
    char rightmove;           // +477 [QL]
    char upmove;              // +478 [QL]
    char _pad;                // +479 alignment padding

    // not communicated over the net at all
    int ping;                 // +480 server to game info for scoreboard
    int pmove_framecount;     // +484
    int jumppad_frame;        // +488
    int entityEventSequence;  // +492

    int freezetime;           // +496 [QL] freeze tag
    int thawtime;             // +500 [QL]
    int thawClientNum_valid;  // +504 [QL]
    int thawClientNum;        // +508 [QL]

    int respawnTime;          // +512 [QL]
    int localPersistant[MAX_PERSISTANT]; // +516 [QL] (64 bytes)
    int roundDamage;          // +580 [QL]
    int roundShots;           // +584 [QL]
    int roundHits;            // +588 [QL]
} playerState_t;

typedef struct usercmd_s {
    int serverTime;
    int angles[3];
    int buttons;
    byte weapon;              // offset 20
    byte weaponPrimary;       // [QL] offset 21 - primary weapon selection (1-14)
    byte fov;                 // [QL] offset 22 - field of view (10-130)
    signed char forwardmove;  // [QL] offset 23 (was 21 in Q3)
    signed char rightmove;    // [QL] offset 24 (was 22 in Q3)
    signed char upmove;       // [QL] offset 25 (was 23 in Q3)
} usercmd_t;

#endif
