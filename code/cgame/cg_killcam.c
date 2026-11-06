/*
===========================================================================
Killcam: after our death, replay the last seconds from behind the killer.

The idea of a client-side snapshot history and the camera come from
quake3-killcam-mod by WofWca, through Devotion. The frame works differently:
there is one pass, not two. While the replay runs, no live scene is drawn, new
snapshots only go to the history and their server commands and obituaries run.
When it ends, the live state is rebuilt from the newest snapshot. So nothing
here costs anything with no replay running, and cg and cg_entities stay direct.

Only what the server already sent us is shown: a killer who was out of our PVS
is not in the history, and the camera then stays on the victim.
===========================================================================
*/
#include "cg_local.h"
#include "superhud/superhud.h"

#define KC_START_DELAY_MS   1000    /* from the death to the start of the replay */
#define KC_PREROLL_MS       2500    /* replayed before the death */
#define KC_POSTROLL_MS      2000    /* replayed after the death */
#define KC_FADE_MS          250     /* to black and back, on each switch of the view */

#define KC_RING_BYTES       (4 * 1024 * 1024)
#define KC_MAX_RECORDS      1024
#define KC_MAX_SEEN_OBITS   16

#define KC_DEAD_TURN_SPEED  0.008f  /* share of the way to the killer per millisecond */

#define KC_CAM_TURN_SPEED   0.012f  /* share of the way to the aim per millisecond */

#define KC_CAM_RANGE        50
#define KC_CAM_HEIGHT       24
#define KC_CAM_SIDE         (-15)

typedef struct
{
	int offset;
	int size;
	int serverTime;
	int serverCommandSequence;
} kcRecord_t;

int      cg_killcamState;
qboolean cg_killcamRingRead;

/* Only the entities a snapshot really holds are stored, not all 256 slots */
static byte        kc_ring[KC_RING_BYTES];
static kcRecord_t  kc_records[KC_MAX_RECORDS];
static int         kc_first;        /* oldest record, counted from the start, not wrapped */
static int         kc_next;         /* the record to be written next */
static int         kc_cursor;       /* the record to be read next */
static int         kc_writeOffset;

static snapshot_t  kc_liveSnapshot;

static int      kc_killer;
static int      kc_spawnCount;
static int      kc_deathTime;
static int      kc_startTime;
static int      kc_delay;
static qboolean kc_stop;

/* The camera on our body: follows the killer until the replay, then stays as it was */
static float    kc_deadYaw;
static float    kc_deadPitch;
static qboolean kc_deadViewValid;

static float    kc_camYaw;
static float    kc_camPitch;
static qboolean kc_camValid;

static int kc_liveTime;
static int kc_fadeInStart;

static int kc_seenObits[KC_MAX_SEEN_OBITS];
static int kc_numSeenObits;

#define KC_RECORD(index) (&kc_records[(index) % KC_MAX_RECORDS])

static void CG_KillcamRingPut(const snapshot_t* snap)
{
	const int   headSize = (const char*)snap->entities - (const char*)snap;
	const int   entitiesSize = snap->numEntities * sizeof(entityState_t);
	const int   size = headSize + entitiesSize;
	kcRecord_t* record;

	if (kc_writeOffset + size > KC_RING_BYTES)
	{
		kc_writeOffset = 0;
	}

	/* Free what the new record overwrites. The oldest records are always next in line. */
	while (kc_first < kc_next)
	{
		const kcRecord_t* oldest = KC_RECORD(kc_first);
		if (kc_next - kc_first < KC_MAX_RECORDS &&
		        (oldest->offset >= kc_writeOffset + size || oldest->offset + oldest->size <= kc_writeOffset))
		{
			break;
		}
		kc_first++;
	}

	record = KC_RECORD(kc_next);
	record->offset = kc_writeOffset;
	record->size = size;
	record->serverTime = snap->serverTime;
	record->serverCommandSequence = snap->serverCommandSequence;
	memcpy(kc_ring + kc_writeOffset, snap, headSize);
	memcpy(kc_ring + kc_writeOffset + headSize, snap->entities, entitiesSize);

	kc_writeOffset += size;
	kc_next++;
}

static void CG_KillcamRingGet(int index, snapshot_t* dest)
{
	const kcRecord_t* record = KC_RECORD(index);
	const int         headSize = (const char*)dest->entities - (const char*)dest;

	memcpy(dest, kc_ring + record->offset, headSize);
	memcpy(dest->entities, kc_ring + record->offset + headSize, record->size - headSize);
	dest->numServerCommands = 0;
	dest->serverCommandSequence = record->serverCommandSequence;
}

void CG_KillcamRecordSnapshot(const snapshot_t* snap)
{
	if (cg.demoPlayback || snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)
	{
		return;
	}
	CG_KillcamRingPut(snap);
}

qboolean CG_KillcamReadRing(snapshot_t* dest)
{
	if (kc_cursor < kc_first)
	{
		kc_cursor = kc_first;
	}
	if (kc_cursor >= kc_next)
	{
		return qfalse;
	}
	CG_KillcamRingGet(kc_cursor, dest);
	kc_cursor++;
	return qtrue;
}

void CG_KillcamOnObituary(int target, int attacker)
{
	if (cg_killcamState != CG_KILLCAM_OFF || !cg_killcam.integer || cg.demoPlayback || cg.warmup ||
	        target != cg.clientNum || (cg.snap->ps.pm_flags & PMF_FOLLOW) ||
	        attacker < 0 || attacker >= MAX_CLIENTS || attacker == target)
	{
		return;
	}
	/* The death that ends a round can come in one snapshot with the respawn of the next one */
	if (cg.snap->ps.stats[STAT_HEALTH] > 0)
	{
		return;
	}
	kc_killer = attacker;
	kc_spawnCount = cg.snap->ps.persistant[PERS_SPAWN_COUNT];
	kc_deathTime = cg.time;
	kc_startTime = cg.time + KC_START_DELAY_MS;
	kc_deadViewValid = qfalse;
	cg_killcamState = CG_KILLCAM_PENDING;
}

void CG_KillcamMapRestart(void)
{
	kc_first = kc_next = kc_cursor = kc_writeOffset = 0;
	if (cg_killcamState == CG_KILLCAM_REPLAY)
	{
		kc_stop = qtrue;
	}
	else
	{
		cg_killcamState = CG_KILLCAM_OFF;
	}
}

/* The replay is over for the live game: we are back in it or the match has ended */
static qboolean CG_KillcamLiveWantsStop(const snapshot_t* snap)
{
	return snap->ps.pm_type == PM_INTERMISSION ||
	       (snap->ps.clientNum == cg.clientNum && snap->ps.persistant[PERS_SPAWN_COUNT] != kc_spawnCount);
}

static void CG_KillcamRememberObits(const snapshot_t* snap)
{
	int i;

	kc_numSeenObits = 0;
	for (i = 0; i < snap->numEntities && kc_numSeenObits < KC_MAX_SEEN_OBITS; i++)
	{
		if (snap->entities[i].eType == ET_EVENTS + EV_OBITUARY)
		{
			kc_seenObits[kc_numSeenObits++] = snap->entities[i].number;
		}
	}
}

/* Obituaries of the live game would be lost with the skipped events. An event stays in a few snapshots in a row. */
static void CG_KillcamRunLiveObits(const snapshot_t* snap)
{
	int i, j;

	for (i = 0; i < snap->numEntities; i++)
	{
		const entityState_t* es = &snap->entities[i];
		if (es->eType != ET_EVENTS + EV_OBITUARY)
		{
			continue;
		}
		for (j = 0; j < kc_numSeenObits; j++)
		{
			if (kc_seenObits[j] == es->number)
			{
				break;
			}
		}
		if (j == kc_numSeenObits)
		{
			CG_Obituary((entityState_t*)es);
		}
	}
	CG_KillcamRememberObits(snap);
}

/* The engine keeps only a few snapshots, so the live ones are taken every frame during a replay. cg.time is still live here. */
static void CG_KillcamDrainLiveSnapshots(void)
{
	int latest;

	trap_GetCurrentSnapshotNumber(&latest, &cg.latestSnapshotTime);
	cg.latestSnapshotNum = latest;

	while (CG_ReadEngineSnapshot(&kc_liveSnapshot))
	{
		CG_ExecuteNewServerCommands(kc_liveSnapshot.serverCommandSequence);
		CG_KillcamRunLiveObits(&kc_liveSnapshot);
		CG_KillcamRingPut(&kc_liveSnapshot);
		if (CG_KillcamLiveWantsStop(&kc_liveSnapshot))
		{
			kc_stop = qtrue;
		}
	}
}

static void CG_KillcamBegin(int serverTime)
{
	const int beginTime = kc_deathTime - KC_PREROLL_MS;
	int       index;

	cg_killcamState = CG_KILLCAM_CORPSE;
	kc_fadeInStart = serverTime;

	if (kc_first == kc_next)
	{
		return;
	}
	for (index = kc_first; index < kc_next - 1; index++)
	{
		if (KC_RECORD(index)->serverTime >= beginTime)
		{
			break;
		}
	}
	if (KC_RECORD(index)->serverTime >= kc_deathTime)
	{
		return;
	}

	CG_KillcamRememberObits(cg.nextSnap ? cg.nextSnap : cg.snap);

	CG_KillcamRingGet(index, &cg.activeSnapshots[0]);
	kc_cursor = index + 1;
	kc_delay = serverTime - cg.activeSnapshots[0].serverTime;
	kc_stop = qfalse;
	kc_camValid = qfalse;

	cg_killcamState = CG_KILLCAM_REPLAY;
	cg_killcamRingRead = qtrue;
	cg.time = cg.activeSnapshots[0].serverTime;
	CG_ResyncSnapshot(&cg.activeSnapshots[0]);
}

static void CG_KillcamEnd(int serverTime)
{
	int index = kc_next - 1;

	cg_killcamState = CG_KILLCAM_OFF;
	kc_fadeInStart = serverTime;

	/* Newer ones are read from the ring as usual until it runs out, then the engine takes over */
	while (index > kc_first && KC_RECORD(index)->serverTime > serverTime)
	{
		index--;
	}
	CG_KillcamRingGet(index, &cg.activeSnapshots[0]);
	kc_cursor = index + 1;

	cg.time = serverTime;
	CG_ResyncSnapshot(&cg.activeSnapshots[0]);
	if (cg.snap->ps.stats[STAT_HEALTH] > 0)
	{
		CG_Respawn();
	}
	if (!kc_stop && !CG_KillcamLiveWantsStop(cg.snap))
	{
		cg_killcamState = CG_KILLCAM_CORPSE;
	}
}

void CG_KillcamFrame(int serverTime)
{
	if (!cg.snap)
	{
		return;
	}

	kc_liveTime = serverTime;

	if (cg_killcamState == CG_KILLCAM_CORPSE)
	{
		if (!cg_killcam.integer || CG_KillcamLiveWantsStop(cg.snap) ||
		        cg.snap->ps.clientNum != cg.clientNum || (cg.snap->ps.pm_flags & PMF_FOLLOW))
		{
			cg_killcamState = CG_KILLCAM_OFF;
		}
		return;
	}

	if (cg_killcamState == CG_KILLCAM_PENDING)
	{
		if (!cg_killcam.integer || CG_KillcamLiveWantsStop(cg.snap))
		{
			cg_killcamState = CG_KILLCAM_OFF;
		}
		else if (serverTime >= kc_startTime)
		{
			CG_KillcamBegin(serverTime);
		}
	}

	if (cg_killcamState == CG_KILLCAM_REPLAY)
	{
		cg.time = serverTime;
		CG_KillcamDrainLiveSnapshots();
		if (kc_stop || !cg_killcam.integer || serverTime - kc_delay >= kc_deathTime + KC_POSTROLL_MS)
		{
			CG_KillcamEnd(serverTime);
		}
		else
		{
			cg.time = serverTime - kc_delay;
			cg.demoPlayback = qtrue;
		}
	}
}

static void CG_KillcamLerpKiller(centity_t* killer)
{
	if (cg.nextSnap && cg.nextSnap->serverTime > cg.snap->serverTime)
	{
		cg.frameInterpolation = (float)(cg.time - cg.snap->serverTime) / (cg.nextSnap->serverTime - cg.snap->serverTime);
	}
	else
	{
		cg.frameInterpolation = 0;
	}
	CG_CalcEntityLerpPositions(killer);
}

/* Angles of the camera on our body. angles come in as the stock view: at the death they already point at the killer. */
void CG_KillcamDeadAngles(vec3_t angles)
{
	centity_t* killer = &cg_entities[kc_killer];

	if (cg.snap->ps.clientNum != cg.clientNum || (cg.snap->ps.pm_flags & PMF_FOLLOW))
	{
		return;
	}

	if (!kc_deadViewValid)
	{
		kc_deadYaw = angles[YAW];
		kc_deadPitch = AngleNormalize180(angles[PITCH]);
		kc_deadViewValid = qtrue;
	}

	if (cg_killcamState == CG_KILLCAM_PENDING && killer->currentValid)
	{
		vec3_t dir, target;
		float  frac = cg.frametime * KC_DEAD_TURN_SPEED;

		if (frac > 1.0f)
		{
			frac = 1.0f;
		}
		CG_KillcamLerpKiller(killer);
		VectorCopy(killer->lerpOrigin, dir);
		dir[2] += DEFAULT_VIEWHEIGHT;
		VectorSubtract(dir, cg.refdef.vieworg, dir);
		vectoangles(dir, target);
		kc_deadYaw = LerpAngle(kc_deadYaw, target[YAW], frac);
		kc_deadPitch = AngleNormalize180(LerpAngle(kc_deadPitch, target[PITCH], frac));
	}

	angles[YAW] = kc_deadYaw;
	angles[PITCH] = kc_deadPitch;
}

/* Behind the killer, looking at the victim. With no killer in the snapshot the view stays as it is. */
void CG_KillcamApplyView(void)
{
	static const vec3_t camMins = { -6, -6, -6 };
	static const vec3_t camMaxs = { 6, 6, 6 };
	centity_t* killer = &cg_entities[kc_killer];
	trace_t    trace;
	vec3_t     eye, target, forward, right, camOrg;

	if (!killer->currentValid || kc_killer == cg.snap->ps.clientNum)
	{
		return;
	}

	CG_KillcamLerpKiller(killer);

	VectorCopy(killer->lerpOrigin, eye);
	eye[2] += DEFAULT_VIEWHEIGHT;

	VectorCopy(cg.predictedPlayerState.origin, target);
	target[2] += DEFAULT_VIEWHEIGHT + KC_CAM_HEIGHT;

	VectorCopy(eye, camOrg);
	camOrg[2] += KC_CAM_HEIGHT;
	CG_Trace(&trace, eye, camMins, camMaxs, camOrg, killer->currentState.number, MASK_SOLID);
	VectorCopy(trace.endpos, eye);

	VectorSubtract(target, eye, forward);
	forward[2] = 0;
	if (VectorNormalize(forward) >= 1)
	{
		right[0] = forward[1];
		right[1] = -forward[0];
		right[2] = 0;
		VectorMA(eye, KC_CAM_SIDE, right, camOrg);
		CG_Trace(&trace, eye, camMins, camMaxs, camOrg, killer->currentState.number, MASK_SOLID);
		VectorCopy(trace.endpos, eye);
	}

	VectorSubtract(target, eye, forward);
	if (VectorNormalize(forward) < 1)
	{
		return;
	}
	vectoangles(forward, cg.refdefViewAngles);

	VectorMA(eye, -(float)KC_CAM_RANGE, forward, camOrg);
	CG_Trace(&trace, eye, camMins, camMaxs, camOrg, killer->currentState.number, MASK_SOLID);
	VectorCopy(trace.endpos, cg.refdef.vieworg);

	if (kc_camValid)
	{
		float frac = cg.frametime * KC_CAM_TURN_SPEED;

		if (frac > 1.0f)
		{
			frac = 1.0f;
		}
		kc_camYaw = LerpAngle(kc_camYaw, cg.refdefViewAngles[YAW], frac);
		kc_camPitch = LerpAngle(kc_camPitch, cg.refdefViewAngles[PITCH], frac);
	}
	else
	{
		kc_camYaw = cg.refdefViewAngles[YAW];
		kc_camPitch = cg.refdefViewAngles[PITCH];
		kc_camValid = qtrue;
	}
	cg.refdefViewAngles[YAW] = kc_camYaw;
	cg.refdefViewAngles[PITCH] = kc_camPitch;

	AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
}

void CG_KillcamDraw2D(void)
{
	CG_DrawBigString(SCREEN_WIDTH / 2, 40, "KILLCAM", 1.0f, DS_HCENTER | DS_SHADOW, 0);

	/* From the moment we die in the replay */
	if (cg.time >= kc_deathTime)
	{
		const char* message = va("You were fragged by %s^7", cgs.clientinfo[kc_killer].name);

		if (!cg_shud.integer || !CG_SHUDDrawFragMessage(message, kc_deathTime))
		{
			CG_DrawBigString(SCREEN_WIDTH / 2, 120, message, 1.0f, DS_HCENTER | DS_SHADOW, 0);
		}
	}
}

void CG_KillcamDrawFade(void)
{
	float  alpha = 0;
	vec4_t color;

	if (cg_killcamState == CG_KILLCAM_PENDING)
	{
		alpha = (float)(kc_liveTime - (kc_startTime - KC_FADE_MS)) / KC_FADE_MS;
	}
	else
	{
		alpha = 1.0f - (float)(kc_liveTime - kc_fadeInStart) / KC_FADE_MS;
		if (cg_killcamState == CG_KILLCAM_REPLAY)
		{
			const float fadeOut = 1.0f - (float)(kc_deathTime + KC_POSTROLL_MS - cg.time) / KC_FADE_MS;
			if (fadeOut > alpha)
			{
				alpha = fadeOut;
			}
		}
	}

	if (alpha <= 0)
	{
		return;
	}
	if (alpha > 1.0f)
	{
		alpha = 1.0f;
	}

	color[0] = color[1] = color[2] = 0;
	color[3] = alpha;
	trap_R_SetColor(color);
	trap_R_DrawStretchPic(0, 0, cgs.glconfig.vidWidth, cgs.glconfig.vidHeight, 0, 0, 0, 0, cgs.media.whiteShader);
	trap_R_SetColor(NULL);
}
