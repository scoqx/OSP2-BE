/*
===========================================================================
Interface between OSP2-BE and the Quake Live movement in game/ql/.

Uses only types both sides share (q_shared.h trace_t / vec3_t): the QL side
is built with Quake Live's playerState_t and usercmd_t, OSP2-BE with its own.
===========================================================================
*/
#ifndef BG_PMOVE_QL_IO_H
#define BG_PMOVE_QL_IO_H

// QL PMF_NO_AUTOHOP: not on the Q3 wire, cg_predict.c sets it from cg_autoHop
#define QLPM_PMF_NO_AUTOHOP 0x40000

#define QLPM_MAXTOUCH 32

typedef struct {
    // in / out: OSP2-BE playerState_t
    void* ps;

    // in: the usercmd
    int serverTime;
    int angles[3];
    int buttons;
    int weapon;
    int forwardmove, rightmove, upmove;

    int tracemask;
    int debugLevel;
    int noFootsteps;
    int gauntletHit;
    void (*trace)(trace_t* results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask);
    int (*pointcontents)(const vec3_t point, int passEntityNum);

    // out
    int numtouch;
    int touchents[QLPM_MAXTOUCH];
    vec3_t mins, maxs;
    int watertype;
    int waterlevel;
    float xyspeed;
} qlpmIo_t;

void QLPM_Run(qlpmIo_t* io);
void QLPM_ParsePmoveInfo(const char* info);
void QLPM_ParseWeaponInfo(const char* info);
void QLPM_SetHoldableItem(int itemNum, int holdableTag);

// last stair step of the QL movement (QL pmove_t stepHeight / stepTime), for view smoothing
extern float qlpm_stepHeight;
extern int qlpm_stepTime;

#endif
