# [Feat]: Cylinder hitboxes for players by metaq3 · Pull Request #42 · metaq3/metarena

[Skip to content](https://github.com/metaq3/metarena/pull/42/changes#start-of-content)

## Pull request toolbar

Open

[[Feat]: Cylinder hitboxes for players](https://github.com/metaq3/metarena/pull/42/changes#top)#42

[metaq3](https://github.com/metaq3)

wants to merge 2 commits into

[master](https://github.com/metaq3/metarena/tree/master)

from

[meta/feat/cylinder-hitboxes](https://github.com/metaq3/metarena/tree/meta/feat/cylinder-hitboxes)

0 of 10 files viewed

## File tree

- build/linux
  - [Makefile](https://github.com/metaq3/metarena/pull/42/changes#diff-a6b526f64968e758c2acbfcce493859db2bebfb133f013b4fe51933c2fbc8ef2)
- code/game
  - [bg_public.h](https://github.com/metaq3/metarena/pull/42/changes#diff-399b1ca70541319d32da5d72eb4da683ed8be23bd6753c2df5213d502a9f5ca9)
  - [g_client.c](https://github.com/metaq3/metarena/pull/42/changes#diff-55c18404adaee7747e04ab86cb5bc7b18b900bd4dd122e3b929d4db38bc1d1fd)
  - [g_cvar.h](https://github.com/metaq3/metarena/pull/42/changes#diff-6cfdf11b0ec3911002c4a71898bb99485bff473b6212cec5504c9b2d15949b9d)
  - [g_geometry.c](https://github.com/metaq3/metarena/pull/42/changes#diff-14678f82ed3b6a02f28d2c82ea414975238ab18df1ff2844e949d98ea5c72553)
  - [g_local.h](https://github.com/metaq3/metarena/pull/42/changes#diff-c7e8b3c7f63de52e20f93031c35af7753f0c0c60053933459f6a7f258db39c97)
  - [g_missile.c](https://github.com/metaq3/metarena/pull/42/changes#diff-5790e9df62fda98e56ec857436eb989621cfcb2e1b4a8dda5564dbf096ea5d92)
  - [g_utils.c](https://github.com/metaq3/metarena/pull/42/changes#diff-7cbe943cd6aea0763a7dccfde560c4a957ac053f32bdd7f2c56da99d1c0104f1)
  - [g_weapon.c](https://github.com/metaq3/metarena/pull/42/changes#diff-03cd23e831eea0e4ee3655d0106582a16eee9b9eca232fe68daf0d1b6f58d06a)
  - [q_shared.h](https://github.com/metaq3/metarena/pull/42/changes#diff-8babc74b17e76c5f8eaea3123bdce47b1b30c9756c0be91abbb141294b11a031)

### [‎build/linux/Makefile‎](https://github.com/metaq3/metarena/pull/42/changes#diff-a6b526f64968e758c2acbfcce493859db2bebfb133f013b4fe51933c2fbc8ef2)

Lines changed: 1 addition & 0 deletions

| Original file line number | Original file line | Diff line number | Diff line change |
| --- | --- | --- | --- |
| `@@ -398,6 +398,7 @@ QAOBJ_ = \` | `@@ -398,6 +398,7 @@ QAOBJ_ = \` | `@@ -398,6 +398,7 @@ QAOBJ_ = \` | `@@ -398,6 +398,7 @@ QAOBJ_ = \` |
| 398 | `$(B)/$(MOD)/game/g_missile.o \` | 398 | `$(B)/$(MOD)/game/g_missile.o \` |
| 399 | `$(B)/$(MOD)/game/g_mover.o \` | 399 | `$(B)/$(MOD)/game/g_mover.o \` |
| 400 | `$(B)/$(MOD)/game/g_rotation.o \` | 400 | `$(B)/$(MOD)/game/g_rotation.o \` |
|  |  | 401 | `+ $(B)/$(MOD)/game/g_geometry.o \` |
| 401 | `$(B)/$(MOD)/game/g_session.o \` | 402 | `$(B)/$(MOD)/game/g_session.o \` |
| 402 | `$(B)/$(MOD)/game/g_spawn.o \` | 403 | `$(B)/$(MOD)/game/g_spawn.o \` |
| 403 | `$(B)/$(MOD)/game/g_svcmds.o \` | 404 | `$(B)/$(MOD)/game/g_svcmds.o \` |
| `` | `` | `` | `` |

### [‎code/game/bg_public.h‎](https://github.com/metaq3/metarena/pull/42/changes#diff-399b1ca70541319d32da5d72eb4da683ed8be23bd6753c2df5213d502a9f5ca9)

Lines changed: 1 addition & 0 deletions

| Original file line number | Original file line | Diff line number | Diff line change |
| --- | --- | --- | --- |
| `@@ -114,6 +114,7 @@` | `@@ -114,6 +114,7 @@` | `@@ -114,6 +114,7 @@` | `@@ -114,6 +114,7 @@` |
| 114 | `#define CS_OSP_CUSTOM_CLIENT2 874` | 114 | `#define CS_OSP_CUSTOM_CLIENT2 874` |
| 115 | `#define CS_OSP2BE_SUPPORTED 887` | 115 | `#define CS_OSP2BE_SUPPORTED 887` |
| 116 | `#define CS_OSP2BE_DISABLED_FEATURES 888` | 116 | `#define CS_OSP2BE_DISABLED_FEATURES 888` |
|  |  | 117 | `+#define CS_CYLINDER_HITBOX_SCALE 911` |
| 117 | `#define XQ3E_ALLOW_FEATURES 1000` | 118 | `#define XQ3E_ALLOW_FEATURES 1000` |
| 118 | `` | 119 | `` |
| 119 | `#define CS_MAX (XQ3E_ALLOW_FEATURES)` | 120 | `#define CS_MAX (XQ3E_ALLOW_FEATURES)` |
| `` | `` | `` | `` |

### [‎code/game/g_client.c‎](https://github.com/metaq3/metarena/pull/42/changes#diff-55c18404adaee7747e04ab86cb5bc7b18b900bd4dd122e3b929d4db38bc1d1fd)

Lines changed: 6 additions & 0 deletions

| Original file line number | Original file line | Diff line number | Diff line change |
| --- | --- | --- | --- |
| `@@ -939,6 +939,12 @@ qboolean ClientUserinfoChanged( int clientNum ) {` | `@@ -939,6 +939,12 @@ qboolean ClientUserinfoChanged( int clientNum ) {` | `@@ -939,6 +939,12 @@ qboolean ClientUserinfoChanged( int clientNum ) {` | `@@ -939,6 +939,12 @@ qboolean ClientUserinfoChanged( int clientNum ) {` |
| 939 | `trap_SetConfigstring( CS_OSP2BE_SUPPORTED, "1" );` | 939 | `trap_SetConfigstring( CS_OSP2BE_SUPPORTED, "1" );` |
| 940 | `trap_SetConfigstring( CS_OSP2BE_DISABLED_FEATURES, va("%i", be_disabledFeatures.integer) );` | 940 | `trap_SetConfigstring( CS_OSP2BE_DISABLED_FEATURES, va("%i", be_disabledFeatures.integer) );` |
| 941 | `` | 941 | `` |
|  |  | 942 | `+ if ( g_playerCylinderHitbox.integer ) {` |
|  |  | 943 | `+ trap_SetConfigstring( CS_CYLINDER_HITBOX_SCALE, va("%f", g_playerCylinderHitboxScale.value) );` |
|  |  | 944 | `+ } else {` |
|  |  | 945 | `+ trap_SetConfigstring( CS_CYLINDER_HITBOX_SCALE, "0" );` |
|  |  | 946 | `+ }` |
|  |  | 947 | `+` |
| 942 | `// Tell about our current physics` | 948 | `// Tell about our current physics` |
| 943 | `trap_SetConfigstring( CS_OSP_SERVER_MODE, PhyMovetypeToOspServerMode() );` | 949 | `trap_SetConfigstring( CS_OSP_SERVER_MODE, PhyMovetypeToOspServerMode() );` |
| 944 | `` | 950 | `` |
| `` | `` | `` | `` |

### [‎code/game/g_cvar.h‎](https://github.com/metaq3/metarena/pull/42/changes#diff-6cfdf11b0ec3911002c4a71898bb99485bff473b6212cec5504c9b2d15949b9d)

Lines changed: 2 additions & 0 deletions

| Original file line number | Original file line | Diff line number | Diff line change |
| --- | --- | --- | --- |
| `@@ -41,6 +41,8 @@ G_CVAR(g_doTeamLocks, "g_doTeamLocks", "0", CVAR_ARCHIVE, 0, qfalse, qfalse)` | `@@ -41,6 +41,8 @@ G_CVAR(g_doTeamLocks, "g_doTeamLocks", "0", CVAR_ARCHIVE, 0, qfalse, qfalse)` | `@@ -41,6 +41,8 @@ G_CVAR(g_doTeamLocks, "g_doTeamLocks", "0", CVAR_ARCHIVE, 0, qfalse, qfalse)` | `@@ -41,6 +41,8 @@ G_CVAR(g_doTeamLocks, "g_doTeamLocks", "0", CVAR_ARCHIVE, 0, qfalse, qfalse)` |
| 41 | `G_CVAR(g_refAutologin, "g_refAutologin", "0", CVAR_ARCHIVE, 0, qfalse, qfalse)` | 41 | `G_CVAR(g_refAutologin, "g_refAutologin", "0", CVAR_ARCHIVE, 0, qfalse, qfalse)` |
| 42 | `G_CVAR(g_refPassword, "g_refPassword", "none", CVAR_ARCHIVE, 0, qfalse, qfalse)` | 42 | `G_CVAR(g_refPassword, "g_refPassword", "none", CVAR_ARCHIVE, 0, qfalse, qfalse)` |
| 43 | `G_CVAR(g_bodyMove, "g_bodyMove", "0", CVAR_ARCHIVE, 0, qfalse, qfalse)` | 43 | `G_CVAR(g_bodyMove, "g_bodyMove", "0", CVAR_ARCHIVE, 0, qfalse, qfalse)` |
|  |  | 44 | `+G_CVAR(g_playerCylinderHitbox, "g_playerCylinderHitbox", "0", CVAR_ARCHIVE, 0, qfalse, qfalse)` |
|  |  | 45 | `+G_CVAR(g_playerCylinderHitboxScale, "g_playerCylinderHitboxScale", "1", CVAR_ARCHIVE, 0, qfalse, qfalse)` |
| 44 | `G_CVAR(g_grenadeKnockbackness, "g_grenadeKnockbackness", "6", CVAR_ARCHIVE, 0,` | 46 | `G_CVAR(g_grenadeKnockbackness, "g_grenadeKnockbackness", "6", CVAR_ARCHIVE, 0,` |
| 45 | `qfalse, qfalse)` | 47 | `qfalse, qfalse)` |
| 46 | `G_CVAR(g_rocketKnockbackness, "g_rocketKnockbackness", "0", CVAR_ARCHIVE, 0,` | 48 | `G_CVAR(g_rocketKnockbackness, "g_rocketKnockbackness", "0", CVAR_ARCHIVE, 0,` |
| `` | `` | `` | `` |

### [‎code/game/g_geometry.c‎](https://github.com/metaq3/metarena/pull/42/changes#diff-14678f82ed3b6a02f28d2c82ea414975238ab18df1ff2844e949d98ea5c72553)

Lines changed: 72 additions & 0 deletions

| Original file line number | Original file line | Diff line number | Diff line change |
| --- | --- | --- | --- |
| `@@ -0,0 +1,72 @@` | `@@ -0,0 +1,72 @@` | `@@ -0,0 +1,72 @@` | `@@ -0,0 +1,72 @@` |
|  |  | 1 | `+#include "g_local.h"` |
|  |  | 2 | `+#include "q_shared.h"` |
|  |  | 3 | `+` |
|  |  | 4 | `+qboolean G_CylinderIntersect(const ray_t ray, const vec3_t cylinderStart, const vec3_t cylinderEnd,` |
|  |  | 5 | `+ vec_t cylinderRadius, vec_t* outDistance, vec3_t outNormal) {` |
|  |  | 6 | `+ vec3_t ba;` |
|  |  | 7 | `+ vec3_t oc;` |
|  |  | 8 | `+ float baba, bard, baoc, k2, k1, k0, h, t, y;` |
|  |  | 9 | `+` |
|  |  | 10 | `+ VectorSubtract(cylinderEnd, cylinderStart, ba);` |
|  |  | 11 | `+ VectorSubtract(ray.origin, cylinderStart, oc);` |
|  |  | 12 | `+` |
|  |  | 13 | `+ baba = VectorLengthSquared(ba);` |
|  |  | 14 | `+ bard = VectorDot(ba, ray.direction);` |
|  |  | 15 | `+ baoc = VectorDot(ba, oc);` |
|  |  | 16 | `+ k2 = baba - bard * bard;` |
|  |  | 17 | `+ k1 = baba * VectorDot(oc, ray.direction) - baoc * bard;` |
|  |  | 18 | `+ k0 = baba * VectorLengthSquared(oc) - baoc * baoc - cylinderRadius * cylinderRadius * baba;` |
|  |  | 19 | `+ h = k1 * k1 - k2 * k0;` |
|  |  | 20 | `+` |
|  |  | 21 | `+ if ( h < 0.0f ) {` |
|  |  | 22 | `+ //no intersection` |
|  |  | 23 | `+ return qfalse;` |
|  |  | 24 | `+ }` |
|  |  | 25 | `+` |
|  |  | 26 | `+ h = sqrt(h);` |
|  |  | 27 | `+ t = -(k1 + h) / k2;` |
|  |  | 28 | `+` |
|  |  | 29 | `+ // body` |
|  |  | 30 | `+ y = baoc + t * bard;` |
|  |  | 31 | `+` |
|  |  | 32 | `+ if( y > 0.0f && y < baba ) {` |
|  |  | 33 | `+ if (outDistance) {` |
|  |  | 34 | `+ *outDistance = t;` |
|  |  | 35 | `+ }` |
|  |  | 36 | `+` |
|  |  | 37 | `+ if (outNormal) {` |
|  |  | 38 | `+ VectorMA(oc, t, ray.direction, outNormal);` |
|  |  | 39 | `+ VectorMA(outNormal, -y/baba, ba, outNormal);` |
|  |  | 40 | `+` |
|  |  | 41 | `+ outNormal[0] /= cylinderRadius;` |
|  |  | 42 | `+ outNormal[1] /= cylinderRadius;` |
|  |  | 43 | `+ outNormal[2] /= cylinderRadius;` |
|  |  | 44 | `+ }` |
|  |  | 45 | `+` |
|  |  | 46 | `+ return qtrue;` |
|  |  | 47 | `+ }` |
|  |  | 48 | `+` |
|  |  | 49 | `+ // caps` |
|  |  | 50 | `+ t = ( ((y<0.0) ? 0.0 : baba) - baoc)/bard;` |
|  |  | 51 | `+` |
|  |  | 52 | `+ if( fabs(k1+k2*t)<h )` |
|  |  | 53 | `+ {` |
|  |  | 54 | `+ if (outDistance) {` |
|  |  | 55 | `+ *outDistance = t;` |
|  |  | 56 | `+ }` |
|  |  | 57 | `+` |
|  |  | 58 | `+ if (outNormal) {` |
|  |  | 59 | `+ float sign = y < 0 ? -1 : 1;` |
|  |  | 60 | `+ float babaSqrt = sqrt(baba);` |
|  |  | 61 | `+` |
|  |  | 62 | `+ outNormal[0] = ba[0] * sign / babaSqrt;` |
|  |  | 63 | `+ outNormal[1] = ba[1] * sign / babaSqrt;` |
|  |  | 64 | `+ outNormal[2] = ba[2] * sign / babaSqrt;` |
|  |  | 65 | `+ }` |
|  |  | 66 | `+` |
|  |  | 67 | `+ return qtrue;` |
|  |  | 68 | `+ }` |
|  |  | 69 | `+` |
|  |  | 70 | `+ //no intersection` |
|  |  | 71 | `+ return qfalse;` |
|  |  | 72 | `+}` |

### [‎code/game/g_local.h‎](https://github.com/metaq3/metarena/pull/42/changes#diff-c7e8b3c7f63de52e20f93031c35af7753f0c0c60053933459f6a7f258db39c97)

Lines changed: 12 additions & 0 deletions

| Original file line number | Original file line | Diff line number | Diff line change |
| --- | --- | --- | --- |
| `@@ -867,6 +867,18 @@ void G_BroadcastServerCommand(int ignoreClient, const char *command);` | `@@ -867,6 +867,18 @@ void G_BroadcastServerCommand(int ignoreClient, const char *command);` | `@@ -867,6 +867,18 @@ void G_BroadcastServerCommand(int ignoreClient, const char *command);` | `@@ -867,6 +867,18 @@ void G_BroadcastServerCommand(int ignoreClient, const char *command);` |
| 867 | `//` | 867 | `//` |
| 868 | `void G_FlushPrint( int clientNum );` | 868 | `void G_FlushPrint( int clientNum );` |
| 869 | `void G_BufferedPrint( int clientNum, char *msg );` | 869 | `void G_BufferedPrint( int clientNum, char *msg );` |
|  |  | 870 | `+int G_HitScanTrace(trace_t *resultsArray, int maxCount, const vec3_t start, const vec3_t end, int passEntityNum, int contentmask);` |
|  |  | 871 | `+` |
|  |  | 872 | `+//` |
|  |  | 873 | `+// g_geometry.c` |
|  |  | 874 | `+//` |
|  |  | 875 | `+typedef struct {` |
|  |  | 876 | `+ vec3_t origin;` |
|  |  | 877 | `+ vec3_t direction;` |
|  |  | 878 | `+} ray_t;` |
|  |  | 879 | `+` |
|  |  | 880 | `+qboolean G_CylinderIntersect(const ray_t ray, const vec3_t cylinderStart, const vec3_t cylinderEnd,` |
|  |  | 881 | `+ vec_t cylinderRadius, vec_t* outDistance, vec3_t outNormal);` |
| 870 | `//` | 882 | `//` |
| 871 | `// g_client.c` | 883 | `// g_client.c` |
| 872 | `//` | 884 | `//` |
| `` | `` | `` | `` |

### [‎code/game/g_missile.c‎](https://github.com/metaq3/metarena/pull/42/changes#diff-5790e9df62fda98e56ec857436eb989621cfcb2e1b4a8dda5564dbf096ea5d92)

Lines changed: 2 additions & 2 deletions

| Original file line number | Original file line | Diff line number | Diff line change |
| --- | --- | --- | --- |
| `@@ -553,11 +553,11 @@ void G_RunMissile( gentity_t *ent ) {` | `@@ -553,11 +553,11 @@ void G_RunMissile( gentity_t *ent ) {` | `@@ -553,11 +553,11 @@ void G_RunMissile( gentity_t *ent ) {` | `@@ -553,11 +553,11 @@ void G_RunMissile( gentity_t *ent ) {` |
| 553 | `passent = ent->r.ownerNum;` | 553 | `passent = ent->r.ownerNum;` |
| 554 | `}` | 554 | `}` |
| 555 | `// trace a line from the previous position to the current position` | 555 | `// trace a line from the previous position to the current position` |
| 556 | `- trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask );` | 556 | `+ G_HitScanTrace( &tr, 1, ent->r.currentOrigin, origin, passent, ent->clipmask );` |
| 557 | `` | 557 | `` |
| 558 | `if ( tr.startsolid \|\| tr.allsolid ) {` | 558 | `if ( tr.startsolid \|\| tr.allsolid ) {` |
| 559 | `// make sure the tr.entityNum is set to the entity we're stuck in` | 559 | `// make sure the tr.entityNum is set to the entity we're stuck in` |
| 560 | `- trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, passent, ent->clipmask );` | 560 | `+ G_HitScanTrace( &tr, 1, ent->r.currentOrigin, ent->r.currentOrigin, passent, ent->clipmask );` |
| 561 | `tr.fraction = 0;` | 561 | `tr.fraction = 0;` |
| 562 | `}` | 562 | `}` |
| 563 | `else {` | 563 | `else {` |
| `` | `` | `` | `` |

### [‎code/game/g_utils.c‎](https://github.com/metaq3/metarena/pull/42/changes#diff-7cbe943cd6aea0763a7dccfde560c4a957ac053f32bdd7f2c56da99d1c0104f1)

Lines changed: 132 additions & 0 deletions

| Original file line number | Original file line | Diff line number | Diff line change |
| --- | --- | --- | --- |
| `@@ -3,6 +3,7 @@` | `@@ -3,6 +3,7 @@` | `@@ -3,6 +3,7 @@` | `@@ -3,6 +3,7 @@` |
| 3 | `// g_utils.c -- misc utility functions for game module` | 3 | `// g_utils.c -- misc utility functions for game module` |
| 4 | `` | 4 | `` |
| 5 | `#include "g_local.h"` | 5 | `#include "g_local.h"` |
|  |  | 6 | `+#include "q_shared.h"` |
| 6 | `` | 7 | `` |
| 7 | `typedef struct {` | 8 | `typedef struct {` |
| 8 | `char oldShader[MAX_QPATH];` | 9 | `char oldShader[MAX_QPATH];` |
| `@@ -568,6 +569,137 @@ void G_FreeEntity( gentity_t *ed ) {` | `@@ -568,6 +569,137 @@ void G_FreeEntity( gentity_t *ed ) {` | `@@ -568,6 +569,137 @@ void G_FreeEntity( gentity_t *ed ) {` | `@@ -568,6 +569,137 @@ void G_FreeEntity( gentity_t *ed ) {` |
| 568 | `ed->inuse = qfalse;` | 569 | `ed->inuse = qfalse;` |
| 569 | `}` | 570 | `}` |
| 570 | `` | 571 | `` |
|  |  | 572 | `+/*` |
|  |  | 573 | `+=================` |
|  |  | 574 | `+G_HitScanTrace` |
|  |  | 575 | `+` |
|  |  | 576 | `+Performs special `trap_Trace` for hitscan weapons. The main difference is in player hitbox handling: it's` |
|  |  | 577 | `+not a "box" now, it's a cylinder made from the bounding box. Therefore, the function should be used only` |
|  |  | 578 | `+for something that can deal direct damage from some "ray".` |
|  |  | 579 | `+` |
|  |  | 580 | `+For convenience purposes, it can perform series of `trap_Trace`, hiding internal entity linkage and` |
|  |  | 581 | `+unlinkage. The number of traced surfaces/entities is limited to `maxCount` though, and provided` |
|  |  | 582 | `+`resultsArray` should have enough capacity for that many trace results.` |
|  |  | 583 | `+` |
|  |  | 584 | `+Returns count of successful traces. Trace results are stored to the preallocated `resultsArray`.` |
|  |  | 585 | `+=================` |
|  |  | 586 | `+*/` |
|  |  | 587 | `+int G_HitScanTrace(trace_t *resultsArray, int maxCount, const vec3_t start, const vec3_t end, int passEntityNum, int contentmask) {` |
|  |  | 588 | `+ // We would possibly scale players' hitboxes, and we don't want some sort of precision error to occur,` |
|  |  | 589 | `+ // so restore everything up instead of using reverse operations.` |
|  |  | 590 | `+ static vec3_t oldMaxs[MAX_CLIENTS];` |
|  |  | 591 | `+ static vec3_t oldMins[MAX_CLIENTS];` |
|  |  | 592 | `+` |
|  |  | 593 | `+ static gentity_t* skippedUnlinked[MAX_CLIENTS];` |
|  |  | 594 | `+` |
|  |  | 595 | `+ trace_t trace;` |
|  |  | 596 | `+ gentity_t *ent;` |
|  |  | 597 | `+ ray_t ray;` |
|  |  | 598 | `+ int count;` |
|  |  | 599 | `+ int i;` |
|  |  | 600 | `+ vec_t length;` |
|  |  | 601 | `+ int unlinkedCount = 0;` |
|  |  | 602 | `+` |
|  |  | 603 | `+ float hitboxScale = g_playerCylinderHitboxScale.value;` |
|  |  | 604 | `+` |
|  |  | 605 | `+ VectorCopy(start, ray.origin);` |
|  |  | 606 | `+ VectorSubtract(end, start, ray.direction);` |
|  |  | 607 | `+ length = VectorNormalize(ray.direction);` |
|  |  | 608 | `+` |
|  |  | 609 | `+ // Scale up hitboxes if opted out. This is for trap_Trace, so we can use scaled cylinder hit collisions.` |
|  |  | 610 | `+ // Doing it here makes sure that every hitbox is properly scaled and things are not screwed up after it.` |
|  |  | 611 | `+ if ( g_playerCylinderHitbox.integer ) {` |
|  |  | 612 | `+ for ( i = 0; i < MAX_CLIENTS; ++i ) {` |
|  |  | 613 | `+ ent = &g_entities[i];` |
|  |  | 614 | `+` |
|  |  | 615 | `+ if (!ent->inuse) continue;` |
|  |  | 616 | `+` |
|  |  | 617 | `+ VectorCopy(ent->r.maxs, oldMaxs[i]);` |
|  |  | 618 | `+ VectorCopy(ent->r.mins, oldMins[i]);` |
|  |  | 619 | `+` |
|  |  | 620 | `+ ent->r.maxs[0] *= hitboxScale;` |
|  |  | 621 | `+ ent->r.maxs[1] *= hitboxScale;` |
|  |  | 622 | `+ ent->r.mins[0] *= hitboxScale;` |
|  |  | 623 | `+ ent->r.mins[1] *= hitboxScale;` |
|  |  | 624 | `+ }` |
|  |  | 625 | `+ }` |
|  |  | 626 | `+` |
|  |  | 627 | `+ for ( count = 0; count < maxCount; count++ ) {` |
|  |  | 628 | `+ trap_Trace(&trace, start, NULL, NULL, end, passEntityNum, contentmask);` |
|  |  | 629 | `+` |
|  |  | 630 | `+ if (trace.entityNum >= ENTITYNUM_MAX_NORMAL) {` |
|  |  | 631 | `+ resultsArray[count] = trace;` |
|  |  | 632 | `+ break;` |
|  |  | 633 | `+ }` |
|  |  | 634 | `+` |
|  |  | 635 | `+ ent = &g_entities[trace.entityNum];` |
|  |  | 636 | `+` |
|  |  | 637 | `+ if ( !g_playerCylinderHitbox.integer ) {` |
|  |  | 638 | `+ resultsArray[count] = trace;` |
|  |  | 639 | `+ } else if ( ent->client ) {` |
|  |  | 640 | `+ vec_t outDistance;` |
|  |  | 641 | `+ vec3_t cylinderStart;` |
|  |  | 642 | `+ vec3_t cylinderEnd;` |
|  |  | 643 | `+ vec_t cylinderRadius;` |
|  |  | 644 | `+ qboolean intersected;` |
|  |  | 645 | `+` |
|  |  | 646 | `+ // Origin is always at the center of the hitbox` |
|  |  | 647 | `+ cylinderStart[0] = ent->client->ps.origin[0];` |
|  |  | 648 | `+ cylinderStart[1] = ent->client->ps.origin[1];` |
|  |  | 649 | `+ cylinderStart[2] = ent->client->ps.origin[2];` |
|  |  | 650 | `+` |
|  |  | 651 | `+ VectorCopy(cylinderStart, cylinderEnd);` |
|  |  | 652 | `+` |
|  |  | 653 | `+ cylinderStart[2] += ent->r.mins[2];` |
|  |  | 654 | `+ cylinderEnd[2] += ent->r.maxs[2];` |
|  |  | 655 | `+` |
|  |  | 656 | `+ // We're assuming that box's side planes are equal to each other` |
|  |  | 657 | `+ cylinderRadius = (ent->r.maxs[0] - ent->r.mins[0]) * 0.5f;` |
|  |  | 658 | `+` |
|  |  | 659 | `+ intersected = G_CylinderIntersect(ray, cylinderStart, cylinderEnd, cylinderRadius, &outDistance, NULL);` |
|  |  | 660 | `+` |
|  |  | 661 | `+ if (intersected) {` |
|  |  | 662 | `+ VectorMA(ray.origin, outDistance, ray.direction, trace.endpos);` |
|  |  | 663 | `+ trace.fraction = outDistance / length;` |
|  |  | 664 | `+` |
|  |  | 665 | `+ resultsArray[count] = trace;` |
|  |  | 666 | `+ } else {` |
|  |  | 667 | `+ // Don't take this miss into account` |
|  |  | 668 | `+ count--;` |
|  |  | 669 | `+ skippedUnlinked[unlinkedCount] = ent;` |
|  |  | 670 | `+ unlinkedCount++;` |
|  |  | 671 | `+ }` |
|  |  | 672 | `+ }` |
|  |  | 673 | `+` |
|  |  | 674 | `+ // Don't take this entity into account in next trace` |
|  |  | 675 | `+ trap_UnlinkEntity(ent);` |
|  |  | 676 | `+ }` |
|  |  | 677 | `+` |
|  |  | 678 | `+ // Link everything up` |
|  |  | 679 | `+ for ( i = 0; i < count; ++i ) {` |
|  |  | 680 | `+ trap_LinkEntity(&g_entities[resultsArray[i].entityNum]);` |
|  |  | 681 | `+ }` |
|  |  | 682 | `+` |
|  |  | 683 | `+ // Those entities were unlinked, but not included in trace results.` |
|  |  | 684 | `+ // Primarily players, for whose our ray has collided with their hitbox,` |
|  |  | 685 | `+ // but cylinder hitbox check returned no intersection.` |
|  |  | 686 | `+ for ( i = 0; i < unlinkedCount; ++i ) {` |
|  |  | 687 | `+ trap_LinkEntity(skippedUnlinked[i]);` |
|  |  | 688 | `+ }` |
|  |  | 689 | `+` |
|  |  | 690 | `+ if ( g_playerCylinderHitbox.integer ) {` |
|  |  | 691 | `+ for ( i = 0; i < MAX_CLIENTS; ++i ) {` |
|  |  | 692 | `+ ent = &g_entities[i];` |
|  |  | 693 | `+` |
|  |  | 694 | `+ if (!ent->inuse) continue;` |
|  |  | 695 | `+` |
|  |  | 696 | `+ VectorCopy(oldMaxs[i], ent->r.maxs);` |
|  |  | 697 | `+ VectorCopy(oldMins[i], ent->r.mins);` |
|  |  | 698 | `+ }` |
|  |  | 699 | `+ }` |
|  |  | 700 | `+` |
|  |  | 701 | `+ return count;` |
|  |  | 702 | `+}` |
| 571 | `` | 703 | `` |
| 572 | `/*` | 704 | `/*` |
| 573 | `=================` | 705 | `=================` |
| `` | `` | `` | `` |

### [‎code/game/g_weapon.c‎](https://github.com/metaq3/metarena/pull/42/changes#diff-03cd23e831eea0e4ee3655d0106582a16eee9b9eca232fe68daf0d1b6f58d06a)

Lines changed: 15 additions & 22 deletions

| Original file line number | Original file line | Diff line number | Diff line change |
| --- | --- | --- | --- |
| `@@ -217,7 +217,7 @@ static void Bullet_Fire( gentity_t *ent, float spread, int damage ) {` | `@@ -217,7 +217,7 @@ static void Bullet_Fire( gentity_t *ent, float spread, int damage ) {` | `@@ -217,7 +217,7 @@ static void Bullet_Fire( gentity_t *ent, float spread, int damage ) {` | `@@ -217,7 +217,7 @@ static void Bullet_Fire( gentity_t *ent, float spread, int damage ) {` |
| 217 | `G_DoTimeShiftFor( ent );` | 217 | `G_DoTimeShiftFor( ent );` |
| 218 | `//unlagged - backward reconciliation #2` | 218 | `//unlagged - backward reconciliation #2` |
| 219 | `` | 219 | `` |
| 220 | `- trap_Trace( &tr, muzzle_origin, NULL, NULL, end, passent, MASK_SHOT );` | 220 | `+ G_HitScanTrace( &tr, 1, muzzle_origin, end, passent, MASK_SHOT );` |
| 221 | `` | 221 | `` |
| 222 | `//unlagged - backward reconciliation #2` | 222 | `//unlagged - backward reconciliation #2` |
| 223 | `// put them back` | 223 | `// put them back` |
| `@@ -331,7 +331,7 @@ static qboolean ShotgunPellet( const vec3_t start, const vec3_t end, gentity_t *` | `@@ -331,7 +331,7 @@ static qboolean ShotgunPellet( const vec3_t start, const vec3_t end, gentity_t *` | `@@ -331,7 +331,7 @@ static qboolean ShotgunPellet( const vec3_t start, const vec3_t end, gentity_t *` | `@@ -331,7 +331,7 @@ static qboolean ShotgunPellet( const vec3_t start, const vec3_t end, gentity_t *` |
| 331 | `VectorCopy( end, tr_end );` | 331 | `VectorCopy( end, tr_end );` |
| 332 | `` | 332 | `` |
| 333 | `for ( i = 0; i < 10; i++ ) {` | 333 | `for ( i = 0; i < 10; i++ ) {` |
| 334 | `- trap_Trace( &tr, tr_start, NULL, NULL, tr_end, passent, MASK_SHOT );` | 334 | `+ G_HitScanTrace( &tr, 1, tr_start, tr_end, passent, MASK_SHOT );` |
| 335 | `traceEnt = &g_entities[ tr.entityNum ];` | 335 | `traceEnt = &g_entities[ tr.entityNum ];` |
| 336 | `` | 336 | `` |
| 337 | `// send bullet impact` | 337 | `// send bullet impact` |
| `@@ -521,9 +521,8 @@ void weapon_railgun_fire( gentity_t *ent ) {` | `@@ -521,9 +521,8 @@ void weapon_railgun_fire( gentity_t *ent ) {` | `@@ -521,9 +521,8 @@ void weapon_railgun_fire( gentity_t *ent ) {` | `@@ -521,9 +521,8 @@ void weapon_railgun_fire( gentity_t *ent ) {` |
| 521 | `int damage;` | 521 | `int damage;` |
| 522 | `int i;` | 522 | `int i;` |
| 523 | `int hits;` | 523 | `int hits;` |
| 524 | `- int unlinked;` | 524 | `+ int count;` |
| 525 | `int passent;` | 525 | `int passent;` |
| 526 | `- gentity_t *unlinkedEntities[MAX_RAIL_HITS];` |  |  |
| 527 | `` | 526 | `` |
| 528 | `damage = g_rgDamage.integer * s_quadFactor;` | 527 | `damage = g_rgDamage.integer * s_quadFactor;` |
| 529 | `` | 528 | `` |
| `@@ -539,14 +538,18 @@ void weapon_railgun_fire( gentity_t *ent ) {` | `@@ -539,14 +538,18 @@ void weapon_railgun_fire( gentity_t *ent ) {` | `@@ -539,14 +538,18 @@ void weapon_railgun_fire( gentity_t *ent ) {` | `@@ -539,14 +538,18 @@ void weapon_railgun_fire( gentity_t *ent ) {` |
| 539 | `//unlagged - backward reconciliation #2` | 538 | `//unlagged - backward reconciliation #2` |
| 540 | `` | 539 | `` |
| 541 | `// trace only against the solids, so the railgun will go through people` | 540 | `// trace only against the solids, so the railgun will go through people` |
| 542 | `- unlinked = 0;` | 541 | `+ count = 0;` |
| 543 | `hits = 0;` | 542 | `hits = 0;` |
| 544 | `passent = ent->s.number;` | 543 | `passent = ent->s.number;` |
| 545 | `- do {` | 544 | `+` |
| 546 | `- trap_Trace( &trace, muzzle_origin, NULL, NULL, end, passent, MASK_SHOT );` | 545 | `+ trace_t traces[MAX_RAIL_HITS];` |
| 547 | `- if ( trace.entityNum >= ENTITYNUM_MAX_NORMAL ) {` | 546 | `+ count = G_HitScanTrace(traces, MAX_RAIL_HITS, muzzle_origin, end, passent, MASK_SHOT);` |
| 548 | `- break;` | 547 | `+` |
| 549 | `- }` | 548 | `+ trace = traces[0];` |
|  |  | 549 | `+` |
|  |  | 550 | `+ for ( i = 0; i < count; i++ ) {` |
|  |  | 551 | `+ trace = traces[i];` |
|  |  | 552 | `+` |
| 550 | `traceEnt = &g_entities[ trace.entityNum ];` | 553 | `traceEnt = &g_entities[ trace.entityNum ];` |
| 551 | `if ( traceEnt->takedamage ) {` | 554 | `if ( traceEnt->takedamage ) {` |
| 552 | `#ifdef MISSIONPACK` | 555 | `#ifdef MISSIONPACK` |
| `@@ -582,23 +585,13 @@ void weapon_railgun_fire( gentity_t *ent ) {` | `@@ -582,23 +585,13 @@ void weapon_railgun_fire( gentity_t *ent ) {` | `@@ -582,23 +585,13 @@ void weapon_railgun_fire( gentity_t *ent ) {` | `@@ -582,23 +585,13 @@ void weapon_railgun_fire( gentity_t *ent ) {` |
| 582 | `if ( trace.contents & CONTENTS_SOLID ) {` | 585 | `if ( trace.contents & CONTENTS_SOLID ) {` |
| 583 | `break; // we hit something solid enough to stop the beam` | 586 | `break; // we hit something solid enough to stop the beam` |
| 584 | `}` | 587 | `}` |
| 585 | `- // unlink this entity, so the next trace will go past it` | 588 | `+ }` |
| 586 | `- trap_UnlinkEntity( traceEnt );` |  |  |
| 587 | `- unlinkedEntities[unlinked] = traceEnt;` |  |  |
| 588 | `- unlinked++;` |  |  |
| 589 | `- } while ( unlinked < MAX_RAIL_HITS );` |  |  |
| 590 | `` | 589 | `` |
| 591 | `//unlagged - backward reconciliation #2` | 590 | `//unlagged - backward reconciliation #2` |
| 592 | `// put them back` | 591 | `// put them back` |
| 593 | `G_UndoTimeShiftFor( ent );` | 592 | `G_UndoTimeShiftFor( ent );` |
| 594 | `//unlagged - backward reconciliation #2` | 593 | `//unlagged - backward reconciliation #2` |
| 595 | `` | 594 | `` |
| 596 | `- // link back in any entities we unlinked` |  |  |
| 597 | `- for ( i = 0 ; i < unlinked ; i++ ) {` |  |  |
| 598 | `- trap_LinkEntity( unlinkedEntities[i] );` |  |  |
| 599 | `- }` |  |  |
| 600 | `-` |  |  |
| 601 | `-` |  |  |
| 602 | `if ( g_instagib.integer && hits == 0 ) {` | 595 | `if ( g_instagib.integer && hits == 0 ) {` |
| 603 | `vec3_t dir;` | 596 | `vec3_t dir;` |
| 604 | `float dist;` | 597 | `float dist;` |
| `@@ -747,7 +740,7 @@ void Weapon_LightningFire( gentity_t *ent ) {` | `@@ -747,7 +740,7 @@ void Weapon_LightningFire( gentity_t *ent ) {` | `@@ -747,7 +740,7 @@ void Weapon_LightningFire( gentity_t *ent ) {` | `@@ -747,7 +740,7 @@ void Weapon_LightningFire( gentity_t *ent ) {` |
| 747 | `G_DoTimeShiftFor( ent );` | 740 | `G_DoTimeShiftFor( ent );` |
| 748 | `//unlagged - backward reconciliation #2` | 741 | `//unlagged - backward reconciliation #2` |
| 749 | `` | 742 | `` |
| 750 | `- trap_Trace( &tr, muzzle_origin, NULL, NULL, end, passent, MASK_SHOT );` | 743 | `+ G_HitScanTrace( &tr, 1, muzzle_origin, end, passent, MASK_SHOT );` |
| 751 | `` | 744 | `` |
| 752 | `//unlagged - backward reconciliation #2` | 745 | `//unlagged - backward reconciliation #2` |
| 753 | `// put them back` | 746 | `// put them back` |
| `` | `` | `` | `` |

### [‎code/game/q_shared.h‎](https://github.com/metaq3/metarena/pull/42/changes#diff-8babc74b17e76c5f8eaea3123bdce47b1b30c9756c0be91abbb141294b11a031)

Lines changed: 3 additions & 0 deletions

| Original file line number | Original file line | Diff line number | Diff line change |
| --- | --- | --- | --- |
| `@@ -430,6 +430,9 @@ void ByteToDir(int b, vec3_t dir);` | `@@ -430,6 +430,9 @@ void ByteToDir(int b, vec3_t dir);` | `@@ -430,6 +430,9 @@ void ByteToDir(int b, vec3_t dir);` | `@@ -430,6 +430,9 @@ void ByteToDir(int b, vec3_t dir);` |
| 430 | `#define VectorMA(v, s, b, o) \` | 430 | `#define VectorMA(v, s, b, o) \` |
| 431 | `((o)[0] = (v)[0] + (b)[0] * (s), (o)[1] = (v)[1] + (b)[1] * (s), \` | 431 | `((o)[0] = (v)[0] + (b)[0] * (s), (o)[1] = (v)[1] + (b)[1] * (s), \` |
| 432 | `(o)[2] = (v)[2] + (b)[2] * (s))` | 432 | `(o)[2] = (v)[2] + (b)[2] * (s))` |
|  |  | 433 | `+#define VectorDot(a, b) \` |
|  |  | 434 | `+ ((a)[0] * (b)[0] + (a)[1] * (b)[1] + (a)[2] * (b)[2])` |
|  |  | 435 | `+` |
| 433 | `` | 436 | `` |
| 434 | `#else` | 437 | `#else` |
| 435 | `` | 438 | `` |
| `` | `` | `` | `` |