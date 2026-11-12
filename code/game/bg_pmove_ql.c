/*
===========================================================================
Quake Live player movement for OSP2-BE (qalive servers)

On a qalive (Quake Live) server OSP2-BE predicts with the server's own
movement: game/ql/ holds qalive's bg_pmove.c / bg_slidemove.c, copied by
tools/sync_ql_pmove.sh and built with Quake Live's playerState_t. This file
hands them OSP2-BE's pmove_t; game/ql/ql_glue.c converts the playerState
both ways exactly like the server packs it for Quake III clients.
===========================================================================
*/

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_pmove_ql.h"

static qboolean qlpm_itemsReady;

// the held item arrives as an OSP2-BE item index: tell the QL side which ones are holdables
static void QLPM_RegisterHoldables(void) {
    int i;

    for (i = 1; i < bg_numItems; i++) {
        if (bg_itemlist[i].giType == IT_HOLDABLE) {
            QLPM_SetHoldableItem(i, bg_itemlist[i].giTag);
        }
    }
    qlpm_itemsReady = qtrue;
}

void QLPM_Pmove(pmove_t* pmove) {
    qlpmIo_t io;

    if (!qlpm_itemsReady) {
        QLPM_RegisterHoldables();
    }

    memset(&io, 0, sizeof(io));
    io.ps = pmove->ps;
    io.serverTime = pmove->cmd.serverTime;
    io.angles[0] = pmove->cmd.angles[0];
    io.angles[1] = pmove->cmd.angles[1];
    io.angles[2] = pmove->cmd.angles[2];
    io.buttons = pmove->cmd.buttons;
    io.weapon = pmove->cmd.weapon;
    io.forwardmove = pmove->cmd.forwardmove;
    io.rightmove = pmove->cmd.rightmove;
    io.upmove = pmove->cmd.upmove;
    io.tracemask = pmove->tracemask;
    io.debugLevel = pmove->debugLevel;
    io.noFootsteps = pmove->noFootsteps;
    io.gauntletHit = pmove->gauntletHit;
    io.trace = pmove->trace;
    io.pointcontents = pmove->pointcontents;

    QLPM_Run(&io);

    pmove->numtouch = io.numtouch > MAXTOUCH ? MAXTOUCH : io.numtouch;
    memcpy(pmove->touchents, io.touchents, sizeof(pmove->touchents[0]) * pmove->numtouch);
    VectorCopy(io.mins, pmove->mins);
    VectorCopy(io.maxs, pmove->maxs);
    pmove->watertype = io.watertype;
    pmove->waterlevel = io.waterlevel;
    pmove->xyspeed = io.xyspeed;
}
