#include "cg_local.h"
#include "../qcommon/qcommon.h"

qboolean CG_BE_Timer(int msec) {
    static int lastTime = 0;
    int currentTime = cg.time;

    int diff = currentTime - lastTime;
    if (diff < 0) {
        lastTime = currentTime;
        return qfalse;
    }
    if (diff >= msec) {
        lastTime = currentTime;
        return qtrue;
    }
    return qfalse;
}
