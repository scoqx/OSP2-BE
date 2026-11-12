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
#include "../ui/keycodes.h"

#define KC_START_DELAY_MS   1000    /* from the death to the start of the replay */
#define KC_PREROLL_MS       2500    /* replayed before the death */
#define KC_POSTROLL_MS      2000    /* replayed after the death */
#define KC_FADE_MS          250     /* to black and back, on each switch of the view */

#define KC_RING_BYTES       (4 * 1024 * 1024)
#define KC_MAX_RECORDS      1024
#define KC_MAX_SEEN_OBITS   16

#define KC_DEAD_TURN_SPEED  0.008f  /* share of the way to the killer per millisecond */

#define KC_CAM_TURN_SPEED   0.012f  /* share of the way to the aim per millisecond */

/* cg_killcam 2: the killer's aim comes in snapshots, and a straight line between them turns in pieces.
   The view takes the average of that line over a window centred on the moment. The snapshots after it
   are in the record already, so there is no lag. The window is one snapshot interval each way: then the
   corners at the snapshots are gone. A curve through every snapshot was tried and shook more than the line.
   A bigger turn than this between two snapshots is a jump, and the average stops there. */
#define KC_EYES_JUMP_DEG    90.0f
#define KC_EYES_WINDOW_MAX  60      /* ms each way */
#define KC_EYES_SAMPLES     9
#define KC_EYES_KEYS        10

#define KC_CAM_RANGE        50
#define KC_CAM_HEIGHT       24
#define KC_CAM_SIDE         (-15)

/* Killer died in the replay: pull back from the corpse and orbit while looking at it */
#define KC_DEAD_CAM_RANGE   80
#define KC_DEAD_CAM_HEIGHT  28
#define KC_DEAD_ORBIT_SPEED 0.04f   /* degrees of yaw per millisecond */

typedef struct
{
	int offset;
	int size;
	int serverTime;
	int serverCommandSequence;
} kcRecord_t;

int      cg_killcamState;
int      cg_killcamEyesClient = -1;
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
static qboolean kc_skip;
static int      kc_endingAt;      /* live time an early end started to fade out, 0 when not ending */

/* The view held while the replay fades out with the killer gone */
static qboolean kc_holdView;
static qboolean kc_heldViewValid;
static vec3_t   kc_heldOrigin;
static vec3_t   kc_heldAngles;
static vec3_t   kc_heldAxis[3];

/* The camera on our body: follows the killer until the replay, then stays as it was */
static float    kc_deadYaw;
static float    kc_deadPitch;
static qboolean kc_deadViewValid;

static float    kc_camYaw;
static float    kc_camPitch;
static qboolean kc_camValid;

/* First-person / third-person both switch to this when the killer dies mid-replay */
static qboolean kc_deadKillerView;
static int      kc_killerDeadAt;
static float    kc_deadKillerYaw0;

/* The killer's aim in the recorded snapshots around cg.snap, turned into one unbroken curve */
static int   kc_eyesKeyTime[KC_EYES_KEYS];
static float kc_eyesKeyAngle[KC_EYES_KEYS][2];
static int   kc_eyesKeyCount;
static int   kc_eyesKeysFor;        /* the cg.snap time they were gathered for */
static int         kc_eyesHealth;

static int      kc_killerAwards;
static qboolean kc_killerAwardsKnown;

static int kc_liveTime;
static int kc_fadeInStart;

static int kc_seenObits[KC_MAX_SEEN_OBITS];
static int kc_numSeenObits;

/* The last frag of the one watched in the replay, as he saw it */
#define KC_FRAG_MESSAGE_MS  3000
static char kc_fragMessage[MAX_STRING_CHARS];
static int  kc_fragTime;

/* The scoreboard in a replay: only while its key is held there. Before and after, on our body, it is as it always is. */
static qboolean kc_scoresHeld;

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
	/* In freeze modes the dead become spectators soon, and the replay still needs what comes next */
	if (cg.demoPlayback || (snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR && cg_killcamState == CG_KILLCAM_OFF))
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
	if (cg_killcamState != CG_KILLCAM_OFF || !CG_BE_FEATURE_ENABLED(CG_BE_KILLCAM) || cg.demoPlayback || cg.warmup ||
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

/*
A frag of the one watched, from the replayed history (his frags of others too, before or after ours):
the message, and in first person the effect on his weapon and the frag sound, as he had them.
*/
void CG_KillcamReplayObituary(const entityState_t* es)
{
	const int target = es->otherEntityNum;
	const int attacker = es->otherEntityNum2;

	if (cg_killcamState != CG_KILLCAM_REPLAY || attacker != kc_killer || target == attacker ||
	        target < 0 || target >= MAX_CLIENTS)
	{
		return;
	}
	/* in the freeze modes this is a thaw, not a frag */
	if (es->eventParm == MOD_UNKNOWN && cgs.osp.gameTypeFreeze)
	{
		return;
	}

	Com_sprintf(kc_fragMessage, sizeof(kc_fragMessage), "You fragged %s^7", cgs.clientinfo[target].name);
	kc_fragTime = cg.time;

	if (cg_killcam.integer == 2)
	{
		if (cg_fragSound.integer)
		{
			trap_S_StartLocalSound(CG_GetFragSound(), CHAN_LOCAL_SOUND);
		}
		if (cg_fragEffect.integer && cg_fragEffectTime.integer > 0)
		{
			cg.fragWeaponEffectClientNum = attacker;
			cg.fragWeaponEffectStart = cg.time;
			cg.fragWeaponEffectTime = cg.time + cg_fragEffectTime.integer;
		}
	}
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
/* The intermission does not stop it: the death that ends the match is the one most worth seeing, and the
   intermission waits for the replay (it ends by itself where the replayed history reaches the intermission) */
static qboolean CG_KillcamLiveWantsStop(const snapshot_t* snap)
{
	/* In freeze modes the spawn count also changes when the dead become spectators */
	return snap->ps.clientNum == cg.clientNum && snap->ps.persistant[PERS_SPAWN_COUNT] != kc_spawnCount &&
	       snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR;
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
	/* drop zoom: the replay clock jumps, zoomTime would make the fov bounce */
	cg.zoomed = qfalse;
	cg.zoomTime = 0;

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
	kc_skip = qfalse;
	kc_endingAt = 0;
	kc_holdView = qfalse;
	kc_heldViewValid = qfalse;
	kc_killerAwardsKnown = qfalse;
	cg.rewardStack = 0;
	cg.rewardTime = 0;
	/* the replay's clock goes back over our own last hit and pickup: the crosshair would show them again */
	cgs.osp.lastHitTime = 0;
	cgs.osp.lastHitDamage = 0;
	cg.itemPickupBlendTime = 0;
	kc_camValid = qfalse;
	kc_deadKillerView = qfalse;
	kc_killerDeadAt = 0;
	kc_eyesKeysFor = -1;
	kc_eyesHealth = 0;
	cg_killcamEyesClient = -1;
	kc_fragMessage[0] = '\0';
	kc_fragTime = 0;

	cg_killcamState = CG_KILLCAM_REPLAY;
	cg_killcamRingRead = qtrue;
	kc_scoresHeld = qfalse;
	cg.showScores = qfalse;
	cg.showAccuracy = qfalse;
	cg.time = cg.activeSnapshots[0].serverTime;
	CG_ResyncSnapshot(&cg.activeSnapshots[0]);

	/* Fire must reach us and not the server, where it means respawn. Also when it is held already. */
	trap_SendConsoleCommand("-attack\n");
	trap_Key_SetCatcher(trap_Key_GetCatcher() | KEYCATCH_CGAME);
}

static void CG_KillcamEnd(int serverTime)
{
	int index = kc_next - 1;

	cg_killcamState = CG_KILLCAM_OFF;
	cg_killcamEyesClient = -1;
	kc_fadeInStart = serverTime;
	cg.rewardTime = 0;
	if (kc_scoresHeld || cg.showScores)
	{
		kc_scoresHeld = qfalse;
		cg.showScores = qfalse;
		cg.showAccuracy = qfalse;
	}
	cg.deadScoresHidden = qfalse;

	if (!CG_BEChat_IsOpen())
	{
		trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_CGAME);
	}

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
		if (!CG_BE_FEATURE_ENABLED(CG_BE_KILLCAM) || CG_KillcamLiveWantsStop(cg.snap) ||
		        cg.snap->ps.clientNum != cg.clientNum || (cg.snap->ps.pm_flags & PMF_FOLLOW) ||
		        cg.snap->ps.pm_type == PM_INTERMISSION)
		{
			cg_killcamState = CG_KILLCAM_OFF;
		}
		return;
	}

	if (cg_killcamState == CG_KILLCAM_PENDING)
	{
		if (!CG_BE_FEATURE_ENABLED(CG_BE_KILLCAM) || CG_KillcamLiveWantsStop(cg.snap))
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
		/* An early end fades out first, as the planned one does in its last moment */
		if (!kc_endingAt && (kc_stop || kc_skip || !CG_BE_FEATURE_ENABLED(CG_BE_KILLCAM)))
		{
			kc_endingAt = serverTime;
		}
		if ((kc_endingAt && serverTime - kc_endingAt >= KC_FADE_MS) || serverTime - kc_delay >= kc_deathTime + KC_POSTROLL_MS)
		{
			kc_endingAt = 0;
			CG_KillcamEnd(serverTime);
		}
		else
		{
			cg.time = serverTime - kc_delay;
			cg.demoPlayback = qtrue;
			/* A chat window takes the catcher away when it closes */
			if (!(trap_Key_GetCatcher() & KEYCATCH_CGAME))
			{
				trap_Key_SetCatcher(trap_Key_GetCatcher() | KEYCATCH_CGAME);
			}
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

/* The killer's medal in the HUD, with its sound. The server tells only that he has one for now, so no count.
   What he has when we first see him was earned before the replay, so only what comes after that is shown.
   A new one takes the place of the last at once: a replay is too short for them to wait in a queue. */
static void CG_KillcamCheckAwards(const centity_t* killer)
{
	const int awards = killer->currentState.eFlags & (EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET |
	                   EF_AWARD_DEFEND | EF_AWARD_ASSIST | EF_AWARD_CAP);
	const int added = kc_killerAwardsKnown ? awards & ~kc_killerAwards : 0;
	sfxHandle_t sound;

	kc_killerAwards = awards;
	kc_killerAwardsKnown = qtrue;
	if (!added)
	{
		return;
	}

	if (added & EF_AWARD_IMPRESSIVE)
	{
		cg.rewardShader[0] = cgs.media.medalImpressive;
		sound = cgs.media.impressiveSound;
	}
	else if (added & EF_AWARD_EXCELLENT)
	{
		cg.rewardShader[0] = cgs.media.medalExcellent;
		sound = cgs.media.excellentSound;
	}
	else if (added & EF_AWARD_GAUNTLET)
	{
		cg.rewardShader[0] = cgs.media.medalGauntlet;
		sound = cgs.media.humiliationSound;
	}
	else if (added & EF_AWARD_DEFEND)
	{
		cg.rewardShader[0] = cgs.media.medalDefend;
		sound = cgs.media.defendSound;
	}
	else if (added & EF_AWARD_ASSIST)
	{
		cg.rewardShader[0] = cgs.media.medalAssist;
		sound = cgs.media.assistSound;
	}
	else
	{
		cg.rewardShader[0] = cgs.media.medalCapture;
		sound = cgs.media.captureAwardSound;
	}
	cg.rewardCount[0] = 0;
	cg.rewardTime = cg.time;

	if (!(cg_drawRewards.integer & DRAW_REWARDS_NOSOUND))
	{
		trap_S_StartLocalSound(sound, CHAN_ANNOUNCER);
	}
}

static qboolean CG_KillcamEntityAngles(const snapshot_t* snap, int number, vec3_t out)
{
	int i;

	for (i = 0; i < snap->numEntities; i++)
	{
		if (snap->entities[i].number == number)
		{
			VectorCopy(snap->entities[i].apos.trBase, out);
			return qtrue;
		}
	}
	return qfalse;
}

/* The killer's angles in a record. A record is a snapshot cut after its entities. */
static qboolean CG_KillcamRecordAngles(int index, int number, vec3_t out)
{
	return CG_KillcamEntityAngles((const snapshot_t*)(kc_ring + KC_RECORD(index)->offset), number, out);
}

static void CG_KillcamEyesGatherKeys(int number)
{
	int    here = -1;
	int    first, last, i, axis;
	vec3_t a, prev;

	kc_eyesKeysFor = cg.snap->serverTime;
	kc_eyesKeyCount = 0;

	for (i = kc_cursor; i >= kc_first && i >= kc_cursor - 4; i--)
	{
		if (i < kc_next && KC_RECORD(i)->serverTime == cg.snap->serverTime)
		{
			here = i;
			break;
		}
	}
	if (here < 0 || !CG_KillcamRecordAngles(here, number, a))
	{
		return;
	}

	/* Back from cg.snap, then forward, as long as he is there and does not jump */
	first = here;
	VectorCopy(a, prev);
	while (first - 1 >= kc_first && here - (first - 1) < KC_EYES_KEYS / 2)
	{
		vec3_t b;
		if (!CG_KillcamRecordAngles(first - 1, number, b) ||
		        fabs(AngleSubtract(b[YAW], prev[YAW])) >= KC_EYES_JUMP_DEG || fabs(AngleSubtract(b[PITCH], prev[PITCH])) >= KC_EYES_JUMP_DEG)
		{
			break;
		}
		VectorCopy(b, prev);
		first--;
	}
	last = here;
	VectorCopy(a, prev);
	while (last + 1 < kc_next && last + 1 - first < KC_EYES_KEYS)
	{
		vec3_t b;
		if (!CG_KillcamRecordAngles(last + 1, number, b) ||
		        fabs(AngleSubtract(b[YAW], prev[YAW])) >= KC_EYES_JUMP_DEG || fabs(AngleSubtract(b[PITCH], prev[PITCH])) >= KC_EYES_JUMP_DEG)
		{
			break;
		}
		VectorCopy(b, prev);
		last++;
	}

	/* Unbroken: each angle is the one before plus the short way to it */
	for (i = first; i <= last; i++)
	{
		CG_KillcamRecordAngles(i, number, a);
		kc_eyesKeyTime[kc_eyesKeyCount] = KC_RECORD(i)->serverTime;
		for (axis = PITCH; axis <= YAW; axis++)
		{
			kc_eyesKeyAngle[kc_eyesKeyCount][axis] = kc_eyesKeyCount ?
			        kc_eyesKeyAngle[kc_eyesKeyCount - 1][axis] + AngleSubtract(a[axis], prev[axis]) : a[axis];
			prev[axis] = a[axis];
		}
		kc_eyesKeyCount++;
	}
}

/* The straight line between the keys at time t */
static float CG_KillcamEyesLine(int axis, float t)
{
	int j;

	for (j = 0; j < kc_eyesKeyCount - 2 && t > kc_eyesKeyTime[j + 1]; j++)
	{
	}
	return kc_eyesKeyAngle[j][axis] + (kc_eyesKeyAngle[j + 1][axis] - kc_eyesKeyAngle[j][axis]) *
	       (t - kc_eyesKeyTime[j]) / (float)(kc_eyesKeyTime[j + 1] - kc_eyesKeyTime[j]);
}

/* Angles of the killer's aim at cg.time. Without the snapshots around it, the stock straight line. */
static void CG_KillcamEyesAngles(centity_t* killer, vec3_t out)
{
	float t, w, sum[2];
	int   k, axis;

	VectorCopy(killer->lerpAngles, out);
	if (!cg.nextSnap || cg.nextSnap->serverTime <= cg.snap->serverTime)
	{
		return;
	}
	if (kc_eyesKeysFor != cg.snap->serverTime)
	{
		CG_KillcamEyesGatherKeys(killer->currentState.number);
	}
	if (kc_eyesKeyCount < 2)
	{
		return;
	}

	/* The same width each way, as far as the record goes */
	t = cg.snap->serverTime + cg.frameInterpolation * (cg.nextSnap->serverTime - cg.snap->serverTime);
	w = (float)(cg.nextSnap->serverTime - cg.snap->serverTime);
	if (w > KC_EYES_WINDOW_MAX)
	{
		w = KC_EYES_WINDOW_MAX;
	}
	if (w > t - kc_eyesKeyTime[0])
	{
		w = t - kc_eyesKeyTime[0];
	}
	if (w > kc_eyesKeyTime[kc_eyesKeyCount - 1] - t)
	{
		w = kc_eyesKeyTime[kc_eyesKeyCount - 1] - t;
	}
	if (w < 0)
	{
		return;
	}

	sum[0] = sum[1] = 0;
	for (k = 0; k < KC_EYES_SAMPLES; k++)
	{
		const float at = t - w + 2.0f * w * k / (KC_EYES_SAMPLES - 1);
		sum[0] += CG_KillcamEyesLine(PITCH, at);
		sum[1] += CG_KillcamEyesLine(YAW, at);
	}
	for (axis = PITCH; axis <= YAW; axis++)
	{
		out[axis] = AngleMod(sum[axis] / KC_EYES_SAMPLES);
	}
}

/* EV_PAIN of the killer in a first person replay: what our own damage would show. It has no direction. */
void CG_KillcamEyesPain(int health)
{
	int damage = kc_eyesHealth > 0 ? kc_eyesHealth - health : 20;

	kc_eyesHealth = health;
	if (damage < 1)
	{
		damage = 1;
	}
	CG_DamageFeedback(255, 255, damage);
}

qboolean cg_killcamOwnPlum;

/* A replayed snapshot: the killer's hits on us, as he had them. Only ours are known: his playerstate and his
   damage plums never came to us. The damage is what our health and armor lost with him as the attacker
   (PERS_ATTACKER is set on every hit; the fall and the world are ENTITYNUM_WORLD).
   Not ps->damageEvent: the server counts it only with the pain sound, once in 700 ms, and not on the
   killing hit at all, so LG ticks and the killing rail were lost. */
void CG_KillcamHitsOnUs(const playerState_t* ps, const playerState_t* ops)
{
	int    damage;
	vec3_t org;

	if (ps->clientNum != ops->clientNum || ps->persistant[PERS_ATTACKER] != kc_killer || kc_killer == ps->clientNum)
	{
		return;
	}
	damage = (ops->stats[STAT_HEALTH] + ops->stats[STAT_ARMOR]) - (ps->stats[STAT_HEALTH] + ps->stats[STAT_ARMOR]);
	if (damage < 1)
	{
		return;
	}
	/* health and armor over 100 tick down by one a second, the attacker stays from the last hit */
	if (damage <= 2 && (ops->stats[STAT_HEALTH] > 100 || ops->stats[STAT_ARMOR] > 100))
	{
		return;
	}

	CG_PlayPredictedHitSound(damage);

	if (cg_damagePlums.integer & CG_DAMAGEPLUMS_ACTIVE)
	{
		/* where the server puts it: two heights of the box over the origin */
		VectorCopy(ps->origin, org);
		org[2] += (ps->pm_flags & PMF_DUCKED) ? 2 * 16 : 2 * 32;
		cg_killcamOwnPlum = qtrue;
		CG_DamagePlum(cg.predictedPlayerState.clientNum, org, damage);
		cg_killcamOwnPlum = qfalse;
	}
}

/* cg_killcam 2: the killer's eyes. His body is hidden, CG_KillcamAddViewWeapon gives him his gun. */
static void CG_KillcamApplyEyes(centity_t* killer)
{
	const int legsAnim = killer->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	int viewheight = DEFAULT_VIEWHEIGHT;

	if (legsAnim == LEGS_WALKCR || legsAnim == LEGS_IDLECR)
	{
		viewheight = CROUCH_VIEWHEIGHT;
	}

	VectorCopy(killer->lerpOrigin, cg.refdef.vieworg);
	cg.refdef.vieworg[2] += viewheight;
	CG_KillcamEyesAngles(killer, cg.refdefViewAngles);
	AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);

	cg.bobcycle = 0;
	cg.bobfracsin = 0;
	cg.xyspeed = 0;
	cg_killcamEyesClient = kc_killer;
}

/* Killer died during the replay: pull back from the corpse, look at it, slowly orbit until the replay ends.
   Used for both first- and third-person killcam so the view is not stuck inside the body (outlines vanish there). */
static void CG_KillcamApplyDeadKillerView(centity_t* killer)
{
	static const vec3_t camMins = { -6, -6, -6 };
	static const vec3_t camMaxs = { 6, 6, 6 };
	trace_t trace;
	vec3_t  focus, view, forward, desired;
	float   frac;

	VectorCopy(killer->lerpOrigin, focus);
	focus[2] += 12;

	if (!kc_deadKillerView)
	{
		if (kc_camValid)
		{
			kc_deadKillerYaw0 = kc_camYaw;
		}
		else
		{
			vec3_t a;

			/* First person had no cam state: start from the killer's last aim, then turn to the corpse */
			CG_KillcamEyesAngles(killer, a);
			kc_deadKillerYaw0 = a[YAW];
			kc_camYaw = a[YAW];
			kc_camPitch = AngleNormalize180(a[PITCH]);
			kc_camValid = qtrue;
		}
		kc_killerDeadAt = cg.time;
		kc_deadKillerView = qtrue;
	}

	desired[YAW] = kc_deadKillerYaw0 + (cg.time - kc_killerDeadAt) * KC_DEAD_ORBIT_SPEED;
	desired[PITCH] = 15;
	desired[ROLL] = 0;
	AngleVectors(desired, forward, NULL, NULL);

	VectorCopy(focus, view);
	view[2] += KC_DEAD_CAM_HEIGHT;
	VectorMA(view, -(float)KC_DEAD_CAM_RANGE, forward, view);
	CG_Trace(&trace, focus, camMins, camMaxs, view, killer->currentState.number, MASK_SOLID);
	VectorCopy(trace.endpos, cg.refdef.vieworg);

	VectorSubtract(focus, cg.refdef.vieworg, forward);
	if (VectorNormalize(forward) < 1)
	{
		return;
	}
	vectoangles(forward, desired);

	if (kc_camValid)
	{
		frac = cg.frametime * KC_CAM_TURN_SPEED;
		if (frac > 1.0f)
		{
			frac = 1.0f;
		}
		kc_camYaw = LerpAngle(kc_camYaw, desired[YAW], frac);
		kc_camPitch = LerpAngle(kc_camPitch, desired[PITCH], frac);
	}
	else
	{
		kc_camYaw = desired[YAW];
		kc_camPitch = desired[PITCH];
		kc_camValid = qtrue;
	}
	cg.refdefViewAngles[YAW] = kc_camYaw;
	cg.refdefViewAngles[PITCH] = kc_camPitch;
	cg.refdefViewAngles[ROLL] = 0;
	AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);

	cg.bobcycle = 0;
	cg.bobfracsin = 0;
	cg.xyspeed = 0;
}

void CG_KillcamAddViewWeapon(void)
{
	playerState_t ps;
	centity_t*    killer;

	if (cg_killcamEyesClient < 0)
	{
		return;
	}
	killer = &cg_entities[cg_killcamEyesClient];
	if (!cg_drawGun.integer)
	{
		/* The beam with no gun, as CG_AddViewWeapon draws it */
		vec3_t origin;

		VectorMA(cg.refdef.vieworg, -8, cg.refdef.viewaxis[2], origin);
		CG_LightningBolt(killer, origin);
		return;
	}
	if (killer->currentState.weapon == WP_NONE)
	{
		return;
	}

	ps = cg.predictedPlayerState;
	ps.clientNum = cg_killcamEyesClient;
	ps.weapon = killer->currentState.weapon;
	ps.eFlags = killer->currentState.eFlags;
	ps.pm_type = PM_NORMAL;
	ps.persistant[PERS_TEAM] = cgs.clientinfo[cg_killcamEyesClient].team;
	CG_AddViewWeaponFor(&ps, killer);
}

/* Behind the killer, looking at the victim. With no killer in the snapshot the view stays as it is. */
void CG_KillcamApplyView(void)
{
	static const vec3_t camMins = { -6, -6, -6 };
	static const vec3_t camMaxs = { 6, 6, 6 };
	centity_t* killer = &cg_entities[kc_killer];
	trace_t    trace;
	vec3_t     eye, target, forward, right, camOrg;

	cg_killcamEyesClient = -1;

	/* The replayed history has reached the intermission: the players are gone from the map, the replay ends
	   on the last view while it fades out, and the intermission is shown after it */
	if (cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		kc_stop = qtrue;
		kc_holdView = qtrue;
		return;
	}

	if (!killer->currentValid || kc_killer == cg.snap->ps.clientNum)
	{
		/* After our death there is nothing more to show without him: the replay ends here instead of
		   switching to whoever is left in view, on the last view of him while it fades out */
		if (!killer->currentValid && cg.time >= kc_deathTime)
		{
			kc_stop = qtrue;
			kc_holdView = qtrue;
		}
		return;
	}

	CG_KillcamLerpKiller(killer);
	CG_KillcamCheckAwards(killer);

	if (killer->currentState.eFlags & EF_DEAD)
	{
		CG_KillcamApplyDeadKillerView(killer);
		return;
	}
	kc_deadKillerView = qfalse;

	if (cg_killcam.integer == 2)
	{
		CG_KillcamApplyEyes(killer);
		return;
	}

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

const char* CG_KillcamFollowText(void)
{
	return va("killcam %s^7", cgs.clientinfo[kc_killer].name);
}

/* After CG_KillcamApplyView: remember the view, or keep the remembered one while it fades out */
void CG_KillcamHoldView(void)
{
	if (kc_holdView && kc_heldViewValid)
	{
		VectorCopy(kc_heldOrigin, cg.refdef.vieworg);
		VectorCopy(kc_heldAngles, cg.refdefViewAngles);
		AxisCopy(kc_heldAxis, cg.refdef.viewaxis);
		return;
	}
	if (!kc_holdView)
	{
		VectorCopy(cg.refdef.vieworg, kc_heldOrigin);
		VectorCopy(cg.refdefViewAngles, kc_heldAngles);
		AxisCopy(cg.refdef.viewaxis, kc_heldAxis);
		kc_heldViewValid = qtrue;
	}
}

void CG_KillcamSkip(void)
{
	kc_skip = qtrue;
}

/*
What we do ourselves during a replay (the scoreboard, team tasks) is about the live game, not the replayed one:
around it cg.snap is the newest live snapshot, where we are dead, and the time is live. Only the head of
the snapshot is taken: none of this looks at its entities.
*/
typedef struct
{
	snapshot_t*     snap;
	int             time;
	qboolean        demoPlayback;
	playerState_t   predicted;
	qboolean        withPredicted;
} kcLiveSave_t;

static snapshot_t   kc_liveView;
static kcLiveSave_t kc_liveSave;
static qboolean     kc_liveEntered;

static qboolean CG_KillcamLiveEnter(qboolean withPredicted)
{
	const kcRecord_t* record;

	if (kc_liveEntered || kc_first >= kc_next)
	{
		return qfalse;
	}
	record = KC_RECORD(kc_next - 1);
	memcpy(&kc_liveView, kc_ring + record->offset, (const char*)kc_liveView.entities - (const char*)&kc_liveView);
	kc_liveView.numEntities = 0;
	kc_liveView.numServerCommands = 0;

	kc_liveSave.snap = cg.snap;
	kc_liveSave.time = cg.time;
	kc_liveSave.demoPlayback = cg.demoPlayback;
	kc_liveSave.withPredicted = withPredicted;
	if (withPredicted)
	{
		kc_liveSave.predicted = cg.predictedPlayerState;
		cg.predictedPlayerState = kc_liveView.ps;
	}
	cg.snap = &kc_liveView;
	cg.time = kc_liveTime;
	cg.demoPlayback = qfalse;
	kc_liveEntered = qtrue;
	return qtrue;
}

static void CG_KillcamLiveLeave(void)
{
	if (!kc_liveEntered)
	{
		return;
	}
	cg.snap = kc_liveSave.snap;
	cg.time = kc_liveSave.time;
	cg.demoPlayback = kc_liveSave.demoPlayback;
	if (kc_liveSave.withPredicted)
	{
		cg.predictedPlayerState = kc_liveSave.predicted;
	}
	kc_liveEntered = qfalse;
}

/* The replay moves cg.time back by the delay, the chat window lives in live time all along:
   its input grace, key repeat and message ages would wait out the whole delay otherwise */
int CG_KillcamLiveClock(void)
{
	return (cg_killcamState == CG_KILLCAM_REPLAY && !kc_liveEntered) ? kc_liveTime : cg.time;
}

/* A cgame console command during a replay: it is about the live game (tHere takes our death place,
   the team task wheel sends its command this way too), so it runs in the live state */
void CG_KillcamConsoleCommand(void (*function)(void))
{
	if (CG_KillcamLiveEnter(qtrue))
	{
		function();
		CG_KillcamLiveLeave();
		return;
	}
	function();
}

/* For what lives in live time (chat, team task marks): cg.time is live inside, the caller puts the returned one back */
int CG_KillcamLiveTimeEnter(void)
{
	const int saved = cg.time;

	cg.time = CG_KillcamLiveClock();
	return saved;
}

/* The keys that hold the command, also in a bind of several ("+scores; +shudkey2"): ui reads the bindings
   (cgame can not) and lists them in uiCvar, "9 180 ". The exact bind is still found without it. */
static qboolean CG_KillcamKeyListed(int key, const char* uiCvar)
{
	char        list[128];
	const char* p;

	trap_Cvar_VariableStringBuffer(uiCvar, list, sizeof(list));
	for (p = list; *p;)
	{
		while (*p == ' ')
		{
			p++;
		}
		if (!*p)
		{
			break;
		}
		if (atoi(p) == key)
		{
			return qtrue;
		}
		while (*p && *p != ' ')
		{
			p++;
		}
	}
	return qfalse;
}

static qboolean CG_KillcamKeyBound(int key, const char* command, const char* uiCvar)
{
	const int bound = trap_Key_GetKey(command);

	return ((bound > 0 && key == bound) || CG_KillcamKeyListed(key, uiCvar)) ? qtrue : qfalse;
}

/* The replay has the keys and the mouse: the scoreboard and the team task wheel are taken from it here */
void CG_KillcamKeyEvent(int key, qboolean down)
{
	/* No bind runs while we hold the keys: the chat window is opened from here.
	   The key's char event and release then go to the chat, which skips them by its input grace and latch. */
	if (CG_KillcamKeyBound(key, "enterchat", "ui_keysChat"))
	{
		if (down)
		{
			const int saved = CG_KillcamLiveTimeEnter();
			CG_BEChat_Open();
			cg.time = saved;
		}
		return;
	}
	if (CG_KillcamKeyBound(key, "enterchatteam", "ui_keysChatTeam"))
	{
		if (down)
		{
			const int saved = CG_KillcamLiveTimeEnter();
			CG_BEChat_Team_f();
			cg.time = saved;
		}
		return;
	}
	/* say, say_team, tHere, tWarn: ui keeps such a bind in ui_kcBind<key>, it runs as it is.
	   cgame commands in it run in the live state (CG_KillcamConsoleCommand). */
	if (CG_KillcamKeyListed(key, "ui_keysKillcamBinds"))
	{
		if (down)
		{
			trap_SendConsoleCommand(va("vstr ui_kcBind%i\n", key));
		}
		return;
	}
	if (CG_KillcamKeyBound(key, "+scores", "ui_keysScores"))
	{
		if (CG_KillcamLiveEnter(qtrue))
		{
			if (down)
			{
				CG_ScoresDown_f();
			}
			else
			{
				CG_ScoresUp_f();
			}
			kc_scoresHeld = down;
			CG_KillcamLiveLeave();
		}
		return;
	}
	if (CG_KillcamKeyBound(key, "+ttwheel", "ui_keysTtwheel"))
	{
		if (CG_KillcamLiveEnter(qtrue))
		{
			if (down)
			{
				CG_SelwheelMarkStart();
				/* what is under the crosshair is in the replay, not where we are */
				cg.teamMarkers.smartTargetType = 1;
				VectorClear(cg.teamMarkers.smartTargetPos);
				cg.teamMarkers.smartTargetEntityNum = -1;
			}
			else if (cg.ttwheelMarkActive)
			{
				CG_SelwheelMarkStop();
			}
			CG_KillcamLiveLeave();
		}
		return;
	}
	if (down && !cg.ttwheelMarkActive && (key == K_MOUSE1 || key == trap_Key_GetKey("+attack")))
	{
		CG_KillcamSkip();
	}
}

void CG_KillcamMouseEvent(int dx, int dy)
{
	if (cg.ttwheelMarkActive)
	{
		CG_SelwheelMouseMove(dx, dy);
	}
}

/* Over the replay: the live scoreboard and the team task wheel */
void CG_KillcamDrawLive2D(void)
{
	/* the mark we send from the wheel shows up at the top, as when alive: it has gone out */
	{
		const int saved = CG_KillcamLiveTimeEnter();
		CG_DrawTeamTaskSentMarks();
		cg.time = saved;
	}
	if (cg.ttwheelMarkActive)
	{
		CG_UpdateAndDrawSelwheelMark();
	}
	if ((kc_scoresHeld && cg.showScores) || (cg.scoreFadeTime && kc_liveTime - cg.scoreFadeTime < FADE_TIME))
	{
		/* the replayed player state stays: in the live one we are dead, and that holds the scoreboard open */
		if (CG_KillcamLiveEnter(qfalse))
		{
			CG_DrawIntermission();
			CG_KillcamLiveLeave();
		}
	}
	if (cg.ttwheelMarkActive && CG_KillcamLiveEnter(qtrue))
	{
		CG_DrawSelwheelIcons();
		CG_KillcamLiveLeave();
	}
}

void CG_KillcamDraw2D(void)
{
	const char* message = NULL;
	int         fragTime = 0;
	qboolean    followDrawn = qfalse;
	qboolean    fragDrawn = qfalse;

	if (CG_KILLCAM_EYES_ACTIVE())
	{
		CG_DrawCrosshair();
	}

	/* Every frag of the killer as it comes in the replay, as he saw it. Ours stays to the end: it is what the
	   replay is about. Without the event in the history (lost in a gap), ours still shows from our death. */
	if (kc_fragTime && cg.time >= kc_fragTime &&
	        (cg.time - kc_fragTime < KC_FRAG_MESSAGE_MS || cg.time >= kc_deathTime))
	{
		message = kc_fragMessage;
		fragTime = kc_fragTime;
	}
	if (cg.time >= kc_deathTime && (!message || kc_fragTime < kc_deathTime - 1000))
	{
		message = va("You fragged %s^7", cgs.clientinfo[cg.clientNum].name);
		fragTime = kc_deathTime;
	}

	if (cg_shud.integer)
	{
		CG_SHUDDrawKillcam(message, fragTime, &followDrawn, &fragDrawn);
	}
	else
	{
		CG_DrawReward();
	}

	if (!cg_shud.integer && CG_KILLCAM_EYES_ACTIVE())
	{
		/* The stock names draw only in first person */
		cg.renderingThirdPerson = qfalse;
		CG_DrawCrosshairNames();
		cg.renderingThirdPerson = qtrue;
	}

	if (!followDrawn)
	{
		CG_DrawBigString(SCREEN_WIDTH / 2, 40, CG_KillcamFollowText(), 1.0f, DS_HCENTER | DS_SHADOW, 0);
	}
	if (message && !fragDrawn)
	{
		CG_DrawBigString(SCREEN_WIDTH / 2, 120, message, 1.0f, DS_HCENTER | DS_SHADOW, 0);
	}
}

#define KC_BAR_HEIGHT     48.0f
#define KC_BAR_ALPHA      0.35f
#define KC_LABEL_CHAR_W   12.0f
#define KC_LABEL_CHAR_H   16.0f
#define KC_LABEL_ICON     14.0f
#define KC_LABEL_GAP      6.0f
#define KC_LABEL_MARGIN   16.0f
#define KC_LABEL_PULSE_MS 1200

/* The cinema frame of the replay, under the HUD: bars at the top and the bottom, "REPLAY ▶" in the corner */
void CG_KillcamDrawFrame(void)
{
	static const vec4_t barColor = {0, 0, 0, KC_BAR_ALPHA};
	vec4_t      color = {1.0f, 0.1f, 0.1f, 1.0f};
	const float midY = KC_BAR_HEIGHT * 0.5f;
	const int   font = cgFontIndices.killcam;
	const int   flags = DS_PROPORTIONAL | DS_VCENTER | DS_FORCE_COLOR | DS_PLAIN;
	/* top left corner: the word at the margin, the icon after it (its centre in 640 space) */
	const float textX = KC_LABEL_MARGIN;
	const float iconCx = textX + CG_OSPDrawStringNewTextWidth("REPLAY", KC_LABEL_CHAR_W, KC_LABEL_CHAR_H, flags, font) +
	                     KC_LABEL_GAP + KC_LABEL_ICON * 0.5f;

	CG_FillRect(0, 0, SCREEN_WIDTH, KC_BAR_HEIGHT, barColor);
	CG_FillRect(0, SCREEN_HEIGHT - KC_BAR_HEIGHT, SCREEN_WIDTH, KC_BAR_HEIGHT, barColor);

	/* the icon and the word pulse as one, by live time: the replay one jumps back at its start */
	color[3] = 0.65f + 0.35f * sin((float)(kc_liveTime % KC_LABEL_PULSE_MS) * (2.0f * M_PI / KC_LABEL_PULSE_MS));

	if (cgs.media.killcamReplayShader)
	{
		/* square on a wide screen too: the size by height, around its 640 place */
		const float size = KC_LABEL_ICON * cgs.screenYScale_Old;

		trap_R_SetColor(color);
		trap_R_DrawStretchPic(iconCx * cgs.screenXScale_Old - size * 0.5f, midY * cgs.screenYScale_Old - size * 0.5f,
		                      size, size, 0, 0, 1, 1, cgs.media.killcamReplayShader);
		trap_R_SetColor(NULL);
	}

	CG_OSPDrawStringNew(textX, midY, "REPLAY", color, NULL, KC_LABEL_CHAR_W, KC_LABEL_CHAR_H, SCREEN_WIDTH,
	                    flags, font, NULL, NULL, NULL);
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
			/* Ending early */
			if (kc_endingAt)
			{
				const float fadeEnd = (float)(kc_liveTime - kc_endingAt) / KC_FADE_MS;
				if (fadeEnd > alpha)
				{
					alpha = fadeEnd;
				}
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

/*
===========================================================================
cg_autoFollow: always watch someone alive.
- In the modes where the dead watch their team (CA, Freeze): after our death, a living teammate, and the
  next one when that one dies. Elsewhere the follow command would take a player out of the game.
- As a spectator, in any mode: when the one we follow dies, his killer if alive, else the next living
  player by number. Where the dead come back, only if he is not back after a while.
===========================================================================
*/
#define AF_DELAY_MS     1000    /* with no killcam to wait for */
#define AF_NEXT_MS      500     /* after the one we follow dies, where the dead stay dead */
#define AF_RESPAWN_MS   2000    /* the same where they come back: he may be back by then */
#define AF_RETRY_MS     1000
#define AF_DEAD_MS      4000    /* a player who died this recently is not picked: dead, or only just back */

qboolean cg_autoFollowActive;
static int      af_nextTry;
static qboolean af_needNew;
static int      af_victim;
static int      af_killer;
static qboolean af_spectator;   /* following as a spectator, not after our own death */
static int      af_deathTime[MAX_CLIENTS];

static qboolean CG_AutoFollowDeadStay(void)
{
	return CG_OSPIsGameTypeCA(cgs.gametype) || cgs.osp.gameTypeFreeze;
}

static qboolean CG_AutoFollowMode(void)
{
	return !cg.demoPlayback && !cg.warmup && CG_AutoFollowDeadStay();
}

/* A real spectator: in Clan Arena the dead are spectators too, but keep their real team */
static qboolean CG_AutoFollowIsSpectator(void)
{
	const clientInfo_t* ci = &cgs.clientinfo[cg.clientNum];

	return ci->team == TEAM_SPECTATOR && (!CG_OSPIsGameTypeCA(cgs.gametype) || ci->rt == TEAM_SPECTATOR);
}

void CG_AutoFollowOnObituary(int target, int attacker)
{
	if (target >= 0 && target < MAX_CLIENTS)
	{
		af_deathTime[target] = cg.time;
	}
	if (cg.demoPlayback)
	{
		return;
	}
	if (target == cg.clientNum && CG_AutoFollowMode() && !CG_AutoFollowIsSpectator())
	{
		cg_autoFollowActive = qtrue;
		af_spectator = qfalse;
		af_needNew = qtrue;
		af_victim = -1;
		af_killer = -1;
		af_nextTry = cg.time + AF_DELAY_MS;
	}
	else if ((cg.snap->ps.pm_flags & PMF_FOLLOW) && target == cg.snap->ps.clientNum &&
	         ((cg_autoFollowActive && !af_spectator && CG_AutoFollowMode()) || CG_AutoFollowIsSpectator()))
	{
		af_spectator = CG_AutoFollowIsSpectator();
		cg_autoFollowActive = qtrue;
		af_needNew = qtrue;
		af_victim = target;
		af_killer = attacker;
		af_nextTry = cg.time + (CG_AutoFollowDeadStay() ? AF_NEXT_MS : AF_RESPAWN_MS);
	}
}

/* Alive as far as we can tell: in the game, not seen dead, and not just killed */
static qboolean CG_AutoFollowAlive(int client, int team)
{
	const clientInfo_t* ci;
	const centity_t*    cent;

	if (client < 0 || client >= MAX_CLIENTS || client == cg.clientNum || client == af_victim)
	{
		return qfalse;
	}
	ci = &cgs.clientinfo[client];
	if (!ci->infoValid || ci->team == TEAM_SPECTATOR || (team >= 0 && ci->team != team))
	{
		return qfalse;
	}
	if (team >= 0 && ci->health <= 0)
	{
		return qfalse;
	}
	if (af_deathTime[client] && cg.time >= af_deathTime[client] && cg.time - af_deathTime[client] < AF_DEAD_MS)
	{
		return qfalse;
	}
	cent = &cg_entities[client];
	if (cent->currentValid && (cent->currentState.eFlags & EF_DEAD))
	{
		return qfalse;
	}
	return qtrue;
}

/* As a spectator: the killer, else the next one by number after the one who died */
static int CG_AutoFollowPick(void)
{
	const int team = af_spectator ? -1 : cgs.clientinfo[cg.clientNum].team;
	const int start = af_victim >= 0 ? af_victim : cg.clientNum;
	int i;

	if (af_spectator && af_killer != af_victim && CG_AutoFollowAlive(af_killer, team))
	{
		return af_killer;
	}
	for (i = 1; i <= cgs.maxclients; i++)
	{
		const int client = (start + i) % cgs.maxclients;
		if (CG_AutoFollowAlive(client, team))
		{
			return client;
		}
	}
	return -1;
}

void CG_AutoFollowFrame(void)
{
	int next;

	if (!cg_autoFollow.integer || !cg.snap || cg.demoPlayback)
	{
		cg_autoFollowActive = qfalse;
		return;
	}
	if (af_spectator)
	{
		/* Back in the game, or no longer following */
		if (!CG_AutoFollowIsSpectator() || !(cg.snap->ps.pm_flags & PMF_FOLLOW))
		{
			cg_autoFollowActive = qfalse;
			return;
		}
	}
	/* The dead here are spectators with full health in the snapshot, so a team tells who is back in the game */
	else if (!CG_AutoFollowMode() ||
	         (cg.snap->ps.clientNum == cg.clientNum && cg.snap->ps.stats[STAT_HEALTH] > 0 &&
	          cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR))
	{
		cg_autoFollowActive = qfalse;
		return;
	}

	/* The killcam comes first */
	if (cg_killcamState == CG_KILLCAM_PENDING || cg_killcamState == CG_KILLCAM_REPLAY || !af_needNew || cg.time < af_nextTry)
	{
		return;
	}

	/* The server may have put us on someone already, or the one who died is back */
	if ((cg.snap->ps.pm_flags & PMF_FOLLOW) &&
	        (cg.snap->ps.clientNum != af_victim || (cg.snap->ps.stats[STAT_HEALTH] > 0 && cg.snap->ps.pm_type != PM_DEAD)))
	{
		af_needNew = qfalse;
		return;
	}

	af_nextTry = cg.time + AF_RETRY_MS;
	next = CG_AutoFollowPick();
	if (next >= 0)
	{
		trap_SendConsoleCommand(va("follow %i\n", next));
		af_needNew = qfalse;
	}
}
