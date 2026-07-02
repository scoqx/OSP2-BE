/*
===========================================================================
cg_shadowitems.c

Ghost 3D pickups when the real item is absent (cg_shadowItems bitmask).
Data: mapdata QITM1 v2 only (entity spawn origin + CM drop). Requires cg_simpleItems 0.

Copyright (c) 2026 diwoc
===========================================================================
*/

#include "cg_local.h"
#include "../qcommon/qcommon.h"

#define CG_SHADOW_ITEM_MAX          384
#define CG_SHADOW_MODE_COUNT        5
#define CG_SHADOW_MATCH_XY_SQ       (4.0f * 4.0f)
#define CG_SHADOW_MATCH_Z           48.0f
/* Same logical pickup pad (dual weapon spawns ~4u; row health/shards need tighter grouping). */
#define CG_SHADOW_SLOT_XY_SQ        (3.0f * 3.0f)
#define CG_SHADOW_SLOT_Z            64.0f
/* Teamed map items respawn at random on server — cycle ghost preview on pad. */
#define CG_SHADOW_SLOT_ROTATE_MS    2000
#define CG_SHADOW_SLOT_CROSSFADE_MS 500
#define CG_SHADOW_GHOST_ALPHA       80

#define CG_SHADOW_ITEM_DROP_DEPTH   4096.0f
#define CG_SHADOW_QITM1_VERSION     2
#define CG_SHADOW_QITM_FLAG_SUSPENDED 1
#define CG_SHADOW_ITEM_NUDGE_STEP   8.0f
#define CG_SHADOW_ITEM_NUDGE_MAX    512.0f
#define CG_SHADOW_ITEM_MAX_DROP_DELTA 256.0f
/* Wider bind + anchor origin for armor/mega when map CM != server item (e.g. fatal). */
#define CG_SHADOW_ARMOR_MATCH_XY_SQ   (24.0f * 24.0f)

#define CG_SHADOW_ITEMS_TEST_DRAW   0

static int CG_ShadowItems_ServerSpawnToMask(int spawnItems)
{
	int mask;

	mask = 0;
	if (spawnItems & (1 << IT_HEALTH))
	{
		mask |= CG_SHADOW_ITEMS_CAT_HEALTH;
	}
	if (spawnItems & (1 << IT_ARMOR))
	{
		mask |= CG_SHADOW_ITEMS_CAT_ARMOR;
	}
	if (spawnItems & (1 << IT_WEAPON))
	{
		mask |= CG_SHADOW_ITEMS_CAT_WEAPON;
	}
	if (spawnItems & (1 << IT_AMMO))
	{
		mask |= CG_SHADOW_ITEMS_CAT_AMMO;
	}
	if (spawnItems & (1 << IT_POWERUP) || spawnItems & (1 << IT_HOLDABLE)
	        || spawnItems & (1 << IT_PERSISTANT_POWERUP))
	{
		mask |= CG_SHADOW_ITEMS_CAT_POWERUP;
	}
	if (spawnItems & (1 << IT_TEAM))
	{
		mask |= CG_SHADOW_ITEMS_CAT_FLAG;
	}
	return mask;
}

static int CG_ShadowItems_EffectiveMask(void)
{
	int cvarVal;

	cvarVal = cg_shadowItems.integer;
	if (!cvarVal)
	{
		return 0;
	}
	if (cvarVal & CG_SHADOW_ITEMS_IGNORE_SERVER)
	{
		return cvarVal & CG_SHADOW_ITEMS_CATEGORY_MASK;
	}
	if (!cgs.beSpawnItemsKnown)
	{
		return cvarVal & CG_SHADOW_ITEMS_CATEGORY_MASK;
	}
	if (!cgs.beSpawnItems)
	{
		return 0;
	}
	return CG_ShadowItems_ServerSpawnToMask(cgs.beSpawnItems);
}

#define CG_SHADOW_ITEM_SLOT_MAX         128
#define CG_SHADOW_ITEM_SLOT_MEMBERS     16

typedef struct
{
	vec3_t  origin;
	int     modelindex;
	int     shadowCat;
} cg_shadowItemPoint_t;

static cg_shadowItemPoint_t     cg_shadowItemPoints[CG_SHADOW_ITEM_MAX];
static int                      cg_shadowItemCount;
static int                      cg_shadowModeIndices[CG_SHADOW_MODE_COUNT][CG_SHADOW_ITEM_MAX];
static int                      cg_shadowModeCount[CG_SHADOW_MODE_COUNT];
static int                      cg_shadowItemPhaseEnt[CG_SHADOW_ITEM_MAX];
static vec3_t                   cg_shadowItemSyncOrigin[CG_SHADOW_ITEM_MAX];
static qboolean                 cg_shadowItemSyncValid[CG_SHADOW_ITEM_MAX];
static int                      cg_shadowItemSlotId[CG_SHADOW_ITEM_MAX];
static int                      cg_shadowItemSlotMembers[CG_SHADOW_ITEM_SLOT_MAX][CG_SHADOW_ITEM_SLOT_MEMBERS];
static int                      cg_shadowItemSlotMemberCount[CG_SHADOW_ITEM_SLOT_MAX];
static int                      cg_shadowItemSlotCount;
static qboolean                 cg_shadowItemVisibleDraw[CG_SHADOW_ITEM_MAX];
static qboolean                 cg_shadowPointActive[CG_SHADOW_MODE_COUNT][CG_SHADOW_ITEM_MAX];
static int                      cg_shadowModelPointStart[MAX_ITEMS];
static int                      cg_shadowModelPointCount[MAX_ITEMS];
static int                      cg_shadowModelPointIdx[CG_SHADOW_ITEM_MAX];
static qboolean                 cg_shadowItemSlotVisible[CG_SHADOW_ITEM_SLOT_MAX];
static vec3_t                   cg_shadowSlotDrawOrigin[CG_SHADOW_ITEM_SLOT_MAX];
static int                      cg_shadowMatchBestEnt[CG_SHADOW_ITEM_MAX];
static float                    cg_shadowMatchBestAbsDz[CG_SHADOW_ITEM_MAX];
/* Item bob / scale-up phase per pickup pad (slot), not per QITM point. */
static int                      cg_shadowSlotBobMiscTime[CG_SHADOW_ITEM_SLOT_MAX];
/* Last seen live item origin (armor/mega); ghost stays here after pickup. */
static vec3_t                   cg_shadowItemAnchorOrigin[CG_SHADOW_ITEM_MAX];
static qboolean                 cg_shadowItemAnchorValid[CG_SHADOW_ITEM_MAX];

static void CG_ShadowItems_SetSyncFromCent(int pointIdx, centity_t* cent);
static float CG_ShadowItems_MatchXySqForModel(int modelindex);

static int CG_ShadowItems_SlotBobMiscTime(int slot)
{
	if (slot < 0 || slot >= CG_SHADOW_ITEM_SLOT_MAX)
	{
		return cg.time;
	}
	if (!cg_shadowSlotBobMiscTime[slot])
	{
		cg_shadowSlotBobMiscTime[slot] = cg.time;
	}
	return cg_shadowSlotBobMiscTime[slot];
}

static void CG_ShadowItems_BindBobFromCent(int pointIdx, centity_t* cent)
{
	int     slot;

	if (pointIdx < 0 || pointIdx >= CG_SHADOW_ITEM_MAX || !cent)
	{
		return;
	}

	slot = cg_shadowItemSlotId[pointIdx];
	if (slot >= 0 && slot < CG_SHADOW_ITEM_SLOT_MAX)
	{
		cg_shadowSlotBobMiscTime[slot] = cent->miscTime;
	}
}

static void CG_ShadowItems_ApplyItemMatch(
        int idx,
        centity_t* cent,
        qboolean itemVisible)
{
	int     slot;

	cg_shadowMatchBestEnt[idx] = cent->currentState.number;
	CG_ShadowItems_SetSyncFromCent(idx, cent);
	cg_shadowItemVisibleDraw[idx] = itemVisible;
	if (itemVisible)
	{
		CG_ShadowItems_BindBobFromCent(idx, cent);
		slot = cg_shadowItemSlotId[idx];
		if (slot >= 0 && slot < CG_SHADOW_ITEM_SLOT_MAX)
		{
			cg_shadowItemSlotVisible[slot] = qtrue;
		}
	}
}

static void CG_ShadowItems_TryMatchCentToPoint(
        int mask,
        centity_t* cent,
        qboolean itemVisible)
{
	int         j;
	int         idx;
	int         mi;
	int         start;
	int         end;
	int         bestEnt;
	float       dz;
	float       absDz;
	float       dx;
	float       dy;
	const float* world;
	const cg_shadowItemPoint_t* pt;
	const float* mapOrigin;

	mi = cent->currentState.modelindex;
	if (mi < 0 || mi >= MAX_ITEMS || !cg_shadowModelPointCount[mi])
	{
		return;
	}

	world = cent->lerpOrigin;
	start = cg_shadowModelPointStart[mi];
	end = start + cg_shadowModelPointCount[mi];

	for (j = start; j < end; j++)
	{
		idx = cg_shadowModelPointIdx[j];
		pt = &cg_shadowItemPoints[idx];
		if (!(mask & pt->shadowCat))
		{
			continue;
		}

		mapOrigin = pt->origin;
		dx = world[0] - mapOrigin[0];
		dy = world[1] - mapOrigin[1];
		if (dx * dx + dy * dy > CG_ShadowItems_MatchXySqForModel(pt->modelindex))
		{
			continue;
		}
		dz = world[2] - mapOrigin[2];
		if (dz > CG_SHADOW_MATCH_Z || dz < -CG_SHADOW_MATCH_Z)
		{
			continue;
		}

		absDz = (dz >= 0.0f) ? dz : -dz;
		bestEnt = cg_shadowMatchBestEnt[idx];
		if (bestEnt >= 0)
		{
			if (absDz > cg_shadowMatchBestAbsDz[idx])
			{
				continue;
			}
			if (absDz == cg_shadowMatchBestAbsDz[idx]
			        && cent->currentState.number != bestEnt)
			{
				continue;
			}
		}

		cg_shadowMatchBestAbsDz[idx] = absDz;
		CG_ShadowItems_ApplyItemMatch(idx, cent, itemVisible);
	}
}

static void CG_ShadowItems_BuildPickupSlots(void);
static void CG_ShadowItems_BuildModelBuckets(void);

static int CG_ShadowItems_ReadLE16(const byte* p)
{
	return p[0] | (p[1] << 8);
}

static float CG_ShadowItems_ReadFloat(const byte* p)
{
	float f;
	memcpy(&f, p, sizeof(f));
	return f;
}

static void CG_ShadowItems_Clear(void)
{
	cg_shadowItemCount = 0;
	memset(cg_shadowModeCount, 0, sizeof(cg_shadowModeCount));
	memset(cg_shadowItemPhaseEnt, 0, sizeof(cg_shadowItemPhaseEnt));
	memset(cg_shadowItemSyncValid, 0, sizeof(cg_shadowItemSyncValid));
	memset(cg_shadowItemSlotId, 0xff, sizeof(cg_shadowItemSlotId));
	cg_shadowItemSlotCount = 0;
	memset(cg_shadowItemSlotMemberCount, 0, sizeof(cg_shadowItemSlotMemberCount));
	memset(cg_shadowPointActive, 0, sizeof(cg_shadowPointActive));
	memset(cg_shadowModelPointStart, 0, sizeof(cg_shadowModelPointStart));
	memset(cg_shadowModelPointCount, 0, sizeof(cg_shadowModelPointCount));
	memset(cg_shadowSlotBobMiscTime, 0, sizeof(cg_shadowSlotBobMiscTime));
	memset(cg_shadowItemAnchorValid, 0, sizeof(cg_shadowItemAnchorValid));
}

static qboolean CG_ShadowItems_UsesAnchorOrigin(int modelindex)
{
	gitem_t*    item;

	if (modelindex < 1 || modelindex >= bg_numItems)
	{
		return qfalse;
	}
	item = &bg_itemlist[modelindex];
	if (!Q_stricmp(item->classname, "item_health_mega"))
	{
		return qtrue;
	}
	if (item->giType != IT_ARMOR)
	{
		return qfalse;
	}
	if (!Q_stricmp(item->classname, "item_armor_shard"))
	{
		return qfalse;
	}
	return qtrue;
}

static float CG_ShadowItems_MatchXySqForModel(int modelindex)
{
	if (CG_ShadowItems_UsesAnchorOrigin(modelindex))
	{
		return CG_SHADOW_ARMOR_MATCH_XY_SQ;
	}
	return CG_SHADOW_MATCH_XY_SQ;
}

static void CG_ShadowItems_RememberAnchorOrigin(int pointIdx, const vec3_t origin)
{
	const cg_shadowItemPoint_t* pt;

	if (pointIdx < 0 || pointIdx >= CG_SHADOW_ITEM_MAX)
	{
		return;
	}
	pt = &cg_shadowItemPoints[pointIdx];
	if (!CG_ShadowItems_UsesAnchorOrigin(pt->modelindex))
	{
		return;
	}
	VectorCopy(origin, cg_shadowItemAnchorOrigin[pointIdx]);
	cg_shadowItemAnchorValid[pointIdx] = qtrue;
}

static qboolean CG_ShadowItems_TraceItemDrop(
        const vec3_t start,
        int contentsMask,
        vec3_t outOrigin)
{
	trace_t     tr;
	vec3_t      dest;
	vec3_t      mins;
	vec3_t      maxs;

	VectorSet(mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
	VectorSet(maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);
	VectorCopy(start, dest);
	dest[2] = start[2] - CG_SHADOW_ITEM_DROP_DEPTH;

	trap_CM_BoxTrace(&tr, start, dest, mins, maxs, 0, contentsMask);
	if (tr.startsolid || tr.allsolid)
	{
		return qfalse;
	}

	VectorCopy(tr.endpos, outOrigin);
	return qtrue;
}

/*
==================
CG_ShadowItems_DropToFloor

FinishSpawningItem-style drop. Nudge up if startsolid; MASK_SOLID + MASK_PLAYERSOLID
(highest floor) so item ghosts match pits with playerclip floors (e.g. fatal).
==================
*/
static qboolean CG_ShadowItems_DropToFloor(const vec3_t spawnOrigin, qboolean suspended, vec3_t outOrigin)
{
	vec3_t      start;
	vec3_t      solidHit;
	vec3_t      playerHit;
	vec3_t      best;
	qboolean    haveSolid;
	qboolean    havePlayer;
	float       nudge;
	float       drop;

	if (suspended)
	{
		VectorCopy(spawnOrigin, outOrigin);
		return qtrue;
	}

	VectorCopy(spawnOrigin, start);
	haveSolid = qfalse;
	havePlayer = qfalse;

	for (nudge = 0.0f; nudge <= CG_SHADOW_ITEM_NUDGE_MAX; nudge += CG_SHADOW_ITEM_NUDGE_STEP)
	{
		if (CG_ShadowItems_TraceItemDrop(start, MASK_SOLID, solidHit))
		{
			haveSolid = qtrue;
		}
		if (CG_ShadowItems_TraceItemDrop(start, MASK_PLAYERSOLID, playerHit))
		{
			havePlayer = qtrue;
		}

		if (haveSolid || havePlayer)
		{
			break;
		}

		start[2] += CG_SHADOW_ITEM_NUDGE_STEP;
	}

	if (!haveSolid && !havePlayer)
	{
		VectorCopy(spawnOrigin, outOrigin);
		return qfalse;
	}

	if (haveSolid && havePlayer)
	{
		if (solidHit[2] > playerHit[2])
		{
			VectorCopy(solidHit, best);
		}
		else
		{
			VectorCopy(playerHit, best);
		}
	}
	else if (haveSolid)
	{
		VectorCopy(solidHit, best);
	}
	else
	{
		VectorCopy(playerHit, best);
	}

	drop = spawnOrigin[2] - best[2];
	if (drop > CG_SHADOW_ITEM_MAX_DROP_DELTA)
	{
		VectorCopy(spawnOrigin, outOrigin);
		return qfalse;
	}

	VectorCopy(best, outOrigin);
	return qtrue;
}

static qboolean CG_ShadowItems_LoadQITM1Modes(const byte* raw, int rawLen, int pos)
{
	int     mode;
	int     i;
	int     nSpawned;
	int     idx;

	for (mode = 0; mode < CG_SHADOW_MODE_COUNT; mode++)
	{
		if (pos + 2 > rawLen)
		{
			CG_ShadowItems_Clear();
			return qfalse;
		}
		nSpawned = CG_ShadowItems_ReadLE16(raw + pos);
		pos += 2;
		if (nSpawned > CG_SHADOW_ITEM_MAX)
		{
			CG_ShadowItems_Clear();
			return qfalse;
		}
		cg_shadowModeCount[mode] = nSpawned;
		for (i = 0; i < nSpawned; i++)
		{
			if (pos + 2 > rawLen)
			{
				CG_ShadowItems_Clear();
				return qfalse;
			}
			idx = CG_ShadowItems_ReadLE16(raw + pos);
			pos += 2;
			if (idx < 0 || idx >= cg_shadowItemCount)
			{
				CG_ShadowItems_Clear();
				return qfalse;
			}
			cg_shadowModeIndices[mode][i] = idx;
			cg_shadowPointActive[mode][idx] = qtrue;
		}
	}

	return qtrue;
}

static qboolean CG_ShadowItems_LoadQITM1(const byte* raw, int rawLen)
{
	int                     version;
	int                     numPoints;
	int                     pos;
	int                     i;
	vec3_t                  spawn;
	qboolean                suspended;
	cg_shadowItemPoint_t*   pt;

	if (rawLen < 10 || raw[0] != 'Q' || raw[1] != 'I' || raw[2] != 'T' || raw[3] != 'M'
	        || raw[4] != '1')
	{
		return qfalse;
	}

	version = CG_ShadowItems_ReadLE16(raw + 5);
	numPoints = CG_ShadowItems_ReadLE16(raw + 7);
	if (numPoints <= 0 || numPoints > CG_SHADOW_ITEM_MAX)
	{
		return qfalse;
	}
	if (version != CG_SHADOW_QITM1_VERSION)
	{
		return qfalse;
	}

	pos = 9;
	CG_ShadowItems_Clear();

	for (i = 0; i < numPoints; i++)
	{
		if (pos + 16 > rawLen)
		{
			CG_ShadowItems_Clear();
			return qfalse;
		}
		pt = &cg_shadowItemPoints[i];
		spawn[0] = CG_ShadowItems_ReadFloat(raw + pos);
		pos += 4;
		spawn[1] = CG_ShadowItems_ReadFloat(raw + pos);
		pos += 4;
		spawn[2] = CG_ShadowItems_ReadFloat(raw + pos);
		pos += 4;
		pt->modelindex = CG_ShadowItems_ReadLE16(raw + pos);
		pos += 2;
		pt->shadowCat = raw[pos];
		pos += 1;
		suspended = (raw[pos] & CG_SHADOW_QITM_FLAG_SUSPENDED) ? qtrue : qfalse;
		pos += 1;
		CG_ShadowItems_DropToFloor(spawn, suspended, pt->origin);
	}
	cg_shadowItemCount = numPoints;

	if (!CG_ShadowItems_LoadQITM1Modes(raw, rawLen, pos))
	{
		return qfalse;
	}

	CG_ShadowItems_BuildPickupSlots();
	CG_ShadowItems_BuildModelBuckets();
	return qtrue;
}

void CG_ShadowItems_Load(void)
{
	const byte* qitm;
	int         qitmLen;

	CG_ShadowItems_Clear();

	if (!CG_MapBundle_Section("QITM", &qitm, &qitmLen))
	{
		return;
	}

	(void)CG_ShadowItems_LoadQITM1(qitm, qitmLen);
}

static int CG_ShadowItems_ModeIndex(void)
{
	switch (cgs.gametype)
	{
	case GT_TOURNAMENT:
		return 1;
	case GT_SINGLE_PLAYER:
		return 2;
	case GT_TEAM:
		return 3;
	case GT_CTF:
		return 4;
	default:
		return 0;
	}
}

static qboolean CG_ShadowItems_XYMatchesMapPointSq(const vec3_t world, const vec3_t mapOrigin, float xySq)
{
	float   dx;
	float   dy;

	dx = world[0] - mapOrigin[0];
	dy = world[1] - mapOrigin[1];
	return (dx * dx + dy * dy <= xySq);
}

static qboolean CG_ShadowItems_ItemSyncsWithPoint(centity_t* cent, const cg_shadowItemPoint_t* pt)
{
	float   dz;

	if (!cent->currentValid)
	{
		return qfalse;
	}
	if (cent->currentState.eType != ET_ITEM)
	{
		return qfalse;
	}
	if (cent->currentState.modelindex != pt->modelindex)
	{
		return qfalse;
	}
	if (!CG_ShadowItems_XYMatchesMapPointSq(cent->lerpOrigin, pt->origin,
	        CG_ShadowItems_MatchXySqForModel(pt->modelindex)))
	{
		return qfalse;
	}
	dz = cent->lerpOrigin[2] - pt->origin[2];
	if (dz > CG_SHADOW_MATCH_Z || dz < -CG_SHADOW_MATCH_Z)
	{
		return qfalse;
	}
	return qtrue;
}

static void CG_ShadowItems_SetSyncFromCent(int pointIdx, centity_t* cent)
{
	VectorCopy(cent->lerpOrigin, cg_shadowItemSyncOrigin[pointIdx]);
	cg_shadowItemPhaseEnt[pointIdx] = cent->currentState.number;
	cg_shadowItemSyncValid[pointIdx] = qtrue;
	CG_ShadowItems_RememberAnchorOrigin(pointIdx, cent->lerpOrigin);
}

static qboolean CG_ShadowItems_MapOriginsSameSlot(const vec3_t a, const vec3_t b)
{
	float   dx;
	float   dy;
	float   dz;

	dx = a[0] - b[0];
	dy = a[1] - b[1];
	dz = a[2] - b[2];
	if (dx * dx + dy * dy > CG_SHADOW_SLOT_XY_SQ)
	{
		return qfalse;
	}
	if (dz > CG_SHADOW_SLOT_Z || dz < -CG_SHADOW_SLOT_Z)
	{
		return qfalse;
	}
	return qtrue;
}

static qboolean CG_ShadowItems_PadHasVisibleItem(const vec3_t padOrigin)
{
	int     i;

	for (i = 0; i < cg_shadowItemCount; i++)
	{
		if (!cg_shadowItemVisibleDraw[i])
		{
			continue;
		}
		if (CG_ShadowItems_MapOriginsSameSlot(cg_shadowItemPoints[i].origin, padOrigin))
		{
			return qtrue;
		}
	}
	return qfalse;
}

static qboolean CG_ShadowItems_IsRowPickupItem(int modelindex)
{
	gitem_t*    item;

	if (modelindex < 1 || modelindex >= bg_numItems)
	{
		return qfalse;
	}
	item = &bg_itemlist[modelindex];
	if (!Q_stricmp(item->classname, "item_armor_shard"))
	{
		return qtrue;
	}
	if (item->giType == IT_HEALTH)
	{
		if (!Q_stricmp(item->classname, "item_health_mega"))
		{
			return qfalse;
		}
		/* 25hp medkit: fixed spawn, not a row/ladder like small health or shards. */
		if (!Q_stricmp(item->classname, "item_health"))
		{
			return qfalse;
		}
		return qtrue;
	}
	return qfalse;
}

static qboolean CG_ShadowItems_CanSharePickupSlot(
        const cg_shadowItemPoint_t* a,
        const cg_shadowItemPoint_t* b)
{
	if (!CG_ShadowItems_MapOriginsSameSlot(a->origin, b->origin))
	{
		return qfalse;
	}
	if (a->modelindex < 1 || a->modelindex >= bg_numItems
	        || b->modelindex < 1 || b->modelindex >= bg_numItems)
	{
		return qfalse;
	}
	/* item_health (25) never shares a pad (e.g. 13death_xt center shard on same entity origin). */
	if (!Q_stricmp(bg_itemlist[a->modelindex].classname, "item_health")
	        || !Q_stricmp(bg_itemlist[b->modelindex].classname, "item_health"))
	{
		return qfalse;
	}
	if (CG_ShadowItems_IsRowPickupItem(a->modelindex)
	        != CG_ShadowItems_IsRowPickupItem(b->modelindex))
	{
		return qfalse;
	}
	return qtrue;
}

static void CG_ShadowItems_BuildPickupSlots(void)
{
	int     i;
	int     j;
	int     slot;
	int     n;

	for (i = 0; i < cg_shadowItemCount; i++)
	{
		cg_shadowItemSlotId[i] = -1;
	}

	cg_shadowItemSlotCount = 0;

	for (i = 0; i < cg_shadowItemCount; i++)
	{
		const cg_shadowItemPoint_t* pi;

		pi = &cg_shadowItemPoints[i];
		slot = -1;
		for (j = 0; j < cg_shadowItemSlotCount; j++)
		{
			int     k;

			n = cg_shadowItemSlotMemberCount[j];
			for (k = 0; k < n; k++)
			{
				const cg_shadowItemPoint_t* pj;

				pj = &cg_shadowItemPoints[cg_shadowItemSlotMembers[j][k]];
				if (CG_ShadowItems_CanSharePickupSlot(pi, pj))
				{
					slot = j;
					break;
				}
			}
			if (slot >= 0)
			{
				break;
			}
		}

		if (slot < 0)
		{
			if (cg_shadowItemSlotCount >= CG_SHADOW_ITEM_SLOT_MAX)
			{
				continue;
			}
			slot = cg_shadowItemSlotCount;
			cg_shadowItemSlotCount++;
			cg_shadowItemSlotMemberCount[slot] = 0;
		}

		n = cg_shadowItemSlotMemberCount[slot];
		if (n >= CG_SHADOW_ITEM_SLOT_MEMBERS)
		{
			continue;
		}
		cg_shadowItemSlotMembers[slot][n] = i;
		cg_shadowItemSlotMemberCount[slot] = n + 1;
		cg_shadowItemSlotId[i] = slot;
	}

	for (slot = 0; slot < cg_shadowItemSlotCount; slot++)
	{
		int     a;
		int     b;
		int     tmp;

		n = cg_shadowItemSlotMemberCount[slot];
		for (a = 1; a < n; a++)
		{
			tmp = cg_shadowItemSlotMembers[slot][a];
			b = a;
			while (b > 0 && cg_shadowItemSlotMembers[slot][b - 1] > tmp)
			{
				cg_shadowItemSlotMembers[slot][b] = cg_shadowItemSlotMembers[slot][b - 1];
				b--;
			}
			cg_shadowItemSlotMembers[slot][b] = tmp;
		}
	}

	for (slot = 0; slot < cg_shadowItemSlotCount; slot++)
	{
		int     idx;
		int     k;
		float   bestZ;

		n = cg_shadowItemSlotMemberCount[slot];
		if (n <= 0)
		{
			continue;
		}
		idx = cg_shadowItemSlotMembers[slot][0];
		bestZ = cg_shadowItemPoints[idx].origin[2];
		for (k = 1; k < n; k++)
		{
			idx = cg_shadowItemSlotMembers[slot][k];
			if (cg_shadowItemPoints[idx].origin[2] > bestZ)
			{
				bestZ = cg_shadowItemPoints[idx].origin[2];
			}
		}
		idx = cg_shadowItemSlotMembers[slot][0];
		VectorCopy(cg_shadowItemPoints[idx].origin, cg_shadowSlotDrawOrigin[slot]);
		cg_shadowSlotDrawOrigin[slot][2] = bestZ;
	}
}

static int CG_ShadowItems_SlotActivePoints(int slot, int mode, int mask, int* active)
{
	int     nActive;
	int     i;
	int     n;
	int     idx;
	const cg_shadowItemPoint_t* pt;

	nActive = 0;
	if (slot < 0 || slot >= CG_SHADOW_ITEM_SLOT_MAX || !active)
	{
		return 0;
	}

	n = cg_shadowItemSlotMemberCount[slot];
	for (i = 0; i < n; i++)
	{
		idx = cg_shadowItemSlotMembers[slot][i];
		pt = &cg_shadowItemPoints[idx];
		if (!cg_shadowPointActive[mode][idx])
		{
			continue;
		}
		if (!(mask & pt->shadowCat))
		{
			continue;
		}
		if (cg_shadowItemVisibleDraw[idx])
		{
			continue;
		}
		if (nActive >= CG_SHADOW_ITEM_SLOT_MEMBERS)
		{
			break;
		}
		active[nActive] = idx;
		nActive++;
	}

	return nActive;
}

static int CG_ShadowItems_GhostDrawAlpha(int pointIdx, int mode, int mask)
{
	int     slot;
	int     n;
	int     nActive;
	int     active[CG_SHADOW_ITEM_SLOT_MEMBERS];
	int     period;
	int     fade;
	int     phase;
	int     tick;
	int     iCur;
	int     iNext;
	int     blend;
	const cg_shadowItemPoint_t* pt;

	if (pointIdx < 0 || pointIdx >= cg_shadowItemCount)
	{
		return 0;
	}
	if (!cg_shadowPointActive[mode][pointIdx])
	{
		return 0;
	}
	pt = &cg_shadowItemPoints[pointIdx];
	if (!(mask & pt->shadowCat))
	{
		return 0;
	}

	slot = cg_shadowItemSlotId[pointIdx];
	if (slot >= 0 && slot < CG_SHADOW_ITEM_SLOT_MAX && cg_shadowItemSlotVisible[slot])
	{
		return 0;
	}

	if (cg_shadowItemVisibleDraw[pointIdx])
	{
		return 0;
	}
	if (CG_ShadowItems_PadHasVisibleItem(pt->origin))
	{
		return 0;
	}

	n = (slot >= 0 && slot < CG_SHADOW_ITEM_SLOT_MAX)
	    ? cg_shadowItemSlotMemberCount[slot] : 1;
	if (n <= 1)
	{
		return CG_SHADOW_GHOST_ALPHA;
	}

	nActive = CG_ShadowItems_SlotActivePoints(slot, mode, mask, active);
	if (nActive <= 0)
	{
		return 0;
	}
	if (nActive == 1)
	{
		return (pointIdx == active[0]) ? CG_SHADOW_GHOST_ALPHA : 0;
	}

	period = CG_SHADOW_SLOT_ROTATE_MS;
	fade = CG_SHADOW_SLOT_CROSSFADE_MS;
	if (fade > period)
	{
		fade = period;
	}
	if (fade < 1)
	{
		fade = 1;
	}

	tick = cg.time / period;
	iCur = tick % nActive;
	iNext = (iCur + 1) % nActive;
	phase = cg.time % period;

	if (phase < period - fade)
	{
		return (pointIdx == active[iCur]) ? CG_SHADOW_GHOST_ALPHA : 0;
	}

	blend = phase - (period - fade);
	if (pointIdx == active[iCur])
	{
		return (CG_SHADOW_GHOST_ALPHA * (fade - blend)) / fade;
	}
	if (pointIdx == active[iNext])
	{
		return (CG_SHADOW_GHOST_ALPHA * blend) / fade;
	}

	return 0;
}

static void CG_ShadowItems_BuildModelBuckets(void)
{
	int     counts[MAX_ITEMS];
	int     i;
	int     mi;
	int     pos;

	memset(counts, 0, sizeof(counts));
	memset(cg_shadowModelPointStart, 0, sizeof(cg_shadowModelPointStart));
	memset(cg_shadowModelPointCount, 0, sizeof(cg_shadowModelPointCount));

	for (i = 0; i < cg_shadowItemCount; i++)
	{
		mi = cg_shadowItemPoints[i].modelindex;
		if (mi >= 0 && mi < MAX_ITEMS)
		{
			counts[mi]++;
		}
	}

	pos = 0;
	for (mi = 0; mi < MAX_ITEMS; mi++)
	{
		cg_shadowModelPointStart[mi] = pos;
		pos += counts[mi];
		cg_shadowModelPointCount[mi] = counts[mi];
	}

	memset(counts, 0, sizeof(counts));
	for (i = 0; i < cg_shadowItemCount; i++)
	{
		mi = cg_shadowItemPoints[i].modelindex;
		if (mi >= 0 && mi < MAX_ITEMS)
		{
			cg_shadowModelPointIdx[cg_shadowModelPointStart[mi] + counts[mi]] = i;
			counts[mi]++;
		}
	}
}


/*
==================
CG_ShadowItems_UpdateFromWorld

One pass over world items (model buckets + cached entity per point).
==================
*/
static void CG_ShadowItems_UpdateFromWorld(int mode, int mask)
{
	int         i;
	int         idx;
	int         entNum;
	int         bestEnt;
	centity_t*  cent;
	entityState_t* es;
	const cg_shadowItemPoint_t* pt;

	if (!cg.snap)
	{
		for (i = 0; i < cg_shadowModeCount[mode]; i++)
		{
			idx = cg_shadowModeIndices[mode][i];
			if (idx >= 0 && idx < CG_SHADOW_ITEM_MAX)
			{
				cg_shadowItemPhaseEnt[idx] = 0;
				cg_shadowItemSyncValid[idx] = qfalse;
			}
		}
		return;
	}

	memset(cg_shadowItemSlotVisible, 0, sizeof(cg_shadowItemSlotVisible));

	for (idx = 0; idx < cg_shadowItemCount; idx++)
	{
		cg_shadowItemVisibleDraw[idx] = qfalse;
		cg_shadowMatchBestEnt[idx] = -1;
		cg_shadowMatchBestAbsDz[idx] = CG_SHADOW_MATCH_Z + 1.0f;
	}

	for (i = 0; i < cg.numItemEntityIndices; i++)
	{
		cent = &cg_entities[cg.itemEntityIndices[i]];
		CG_ShadowItems_TryMatchCentToPoint(mask, cent, qtrue);
	}

	for (i = 0; i < cg.snap->numEntities; i++)
	{
		es = &cg.snap->entities[i];
		if (es->eType != ET_ITEM)
		{
			continue;
		}
		cent = &cg_entities[es->number];
		if (!cent->currentValid)
		{
			continue;
		}
		CG_ShadowItems_TryMatchCentToPoint(mask, cent,
		                                   (es->eFlags & EF_NODRAW) ? qfalse : qtrue);
	}

	for (i = 0; i < cg_shadowModeCount[mode]; i++)
	{
		idx = cg_shadowModeIndices[mode][i];
		if (idx < 0 || idx >= cg_shadowItemCount)
		{
			continue;
		}
		pt = &cg_shadowItemPoints[idx];
		if (!(mask & pt->shadowCat))
		{
			continue;
		}

		bestEnt = cg_shadowMatchBestEnt[idx];
		if (bestEnt >= 0)
		{
			continue;
		}

		entNum = cg_shadowItemPhaseEnt[idx];
		if (entNum > 0 && CG_ShadowItems_ItemSyncsWithPoint(&cg_entities[entNum], pt))
		{
			cent = &cg_entities[entNum];
			CG_ShadowItems_SetSyncFromCent(idx, cent);
			if (cent->currentState.eFlags & EF_NODRAW)
			{
				cg_shadowItemVisibleDraw[idx] = qfalse;
			}
			else
			{
				int     slot;

				cg_shadowItemVisibleDraw[idx] = qtrue;
				slot = cg_shadowItemSlotId[idx];
				if (slot >= 0 && slot < CG_SHADOW_ITEM_SLOT_MAX)
				{
					cg_shadowItemSlotVisible[slot] = qtrue;
				}
			}
		}
		else
		{
			cg_shadowItemPhaseEnt[idx] = 0;
			cg_shadowItemSyncValid[idx] = qfalse;
		}
	}
}

static void CG_ShadowItems_DrawPoint(int pointIdx, const cg_shadowItemPoint_t* pt, int ghostAlpha)
{
	vec3_t  drawOrigin;
	int     slot;

	if (ghostAlpha <= 0)
	{
		return;
	}

	slot = cg_shadowItemSlotId[pointIdx];
	if (CG_ShadowItems_UsesAnchorOrigin(pt->modelindex)
	        && cg_shadowItemAnchorValid[pointIdx])
	{
		VectorCopy(cg_shadowItemAnchorOrigin[pointIdx], drawOrigin);
	}
	else if (cg_shadowItemSyncValid[pointIdx])
	{
		VectorCopy(cg_shadowItemSyncOrigin[pointIdx], drawOrigin);
	}
	else if (slot >= 0 && slot < CG_SHADOW_ITEM_SLOT_MAX
	         && cg_shadowItemSlotMemberCount[slot] > 1)
	{
		VectorCopy(cg_shadowSlotDrawOrigin[slot], drawOrigin);
	}
	else
	{
		VectorCopy(pt->origin, drawOrigin);
	}

	CG_AddWorldItem3D(drawOrigin, pt->modelindex,
	                  CG_ShadowItems_PhaseEntForPoint(pointIdx),
	                  CG_ShadowItems_SlotBobMiscTime(slot), qtrue, ghostAlpha);
}

void CG_AddShadowItems(void)
{
	int     mask;
	int     mode;
	int     i;
	int     idx;
	const cg_shadowItemPoint_t* pt;

	if (cg_simpleItems.integer)
	{
		return;
	}

	mask = CG_ShadowItems_EffectiveMask();
	if (!mask || !cg_shadowItemCount)
	{
		return;
	}

#if CG_SHADOW_ITEMS_TEST_DRAW
	for (i = 0; i < cg_shadowItemCount; i++)
	{
		pt = &cg_shadowItemPoints[i];
		CG_ShadowItems_DrawPoint(i, pt, CG_SHADOW_GHOST_ALPHA);
	}
	return;
#endif

	mode = CG_ShadowItems_ModeIndex();
	if (mode < 0 || mode >= CG_SHADOW_MODE_COUNT)
	{
		return;
	}

	CG_ShadowItems_UpdateFromWorld(mode, mask);

	for (i = 0; i < cg_shadowModeCount[mode]; i++)
	{
		int         ghostAlpha;

		idx = cg_shadowModeIndices[mode][i];
		if (idx < 0 || idx >= cg_shadowItemCount)
		{
			continue;
		}
		pt = &cg_shadowItemPoints[idx];
		if (!(mask & pt->shadowCat))
		{
			continue;
		}
		if (cg_shadowItemVisibleDraw[idx])
		{
			continue;
		}
		ghostAlpha = CG_ShadowItems_GhostDrawAlpha(idx, mode, mask);
		if (ghostAlpha <= 0)
		{
			continue;
		}
		CG_ShadowItems_DrawPoint(idx, pt, ghostAlpha);
	}
}

int CG_ShadowItems_MapPointIndexForCent(centity_t* cent)
{
	int         mi;
	int         j;
	int         idx;
	int         start;
	int         end;
	int         bestIdx;
	float       dz;
	float       absDz;
	float       bestAbsDz;
	const cg_shadowItemPoint_t* pt;

	if (!cg_shadowItemCount || !cg_shadowItems.integer)
	{
		return -1;
	}
	if (!cent || !cent->currentValid || cent->currentState.eType != ET_ITEM)
	{
		return -1;
	}

	mi = cent->currentState.modelindex;
	if (mi < 0 || mi >= MAX_ITEMS || !cg_shadowModelPointCount[mi])
	{
		return -1;
	}

	bestIdx = -1;
	bestAbsDz = CG_SHADOW_MATCH_Z + 1.0f;
	start = cg_shadowModelPointStart[mi];
	end = start + cg_shadowModelPointCount[mi];

	for (j = start; j < end; j++)
	{
		idx = cg_shadowModelPointIdx[j];
		pt = &cg_shadowItemPoints[idx];
		if (!CG_ShadowItems_ItemSyncsWithPoint(cent, pt))
		{
			continue;
		}
		dz = cent->lerpOrigin[2] - pt->origin[2];
		absDz = (dz >= 0.0f) ? dz : -dz;
		if (bestIdx < 0 || absDz < bestAbsDz
		        || (absDz == bestAbsDz && idx < bestIdx))
		{
			bestIdx = idx;
			bestAbsDz = absDz;
		}
	}

	return bestIdx;
}

int CG_ShadowItems_BobMiscTimeForPoint(int pointIdx)
{
	int     slot;

	if (pointIdx < 0 || pointIdx >= CG_SHADOW_ITEM_MAX)
	{
		return cg.time;
	}
	slot = cg_shadowItemSlotId[pointIdx];
	return CG_ShadowItems_SlotBobMiscTime(slot);
}

int CG_ShadowItems_PhaseEntForPoint(int pointIdx)
{
	int     slot;

	if (pointIdx < 0 || pointIdx >= CG_SHADOW_ITEM_MAX)
	{
		return MAX_CLIENTS;
	}
	slot = cg_shadowItemSlotId[pointIdx];
	if (slot >= 0 && slot < CG_SHADOW_ITEM_SLOT_MAX)
	{
		return MAX_CLIENTS + slot;
	}
	return pointIdx + MAX_CLIENTS;
}

void CG_ShadowItems_TouchLiveItemBob(int pointIdx, centity_t* cent)
{
	if (!cent)
	{
		return;
	}
	CG_ShadowItems_BindBobFromCent(pointIdx, cent);
	CG_ShadowItems_RememberAnchorOrigin(pointIdx, cent->lerpOrigin);
}

void CG_ShadowItems_OnItemRespawnEvent(centity_t* cent)
{
	int     mapPoint;

	if (!cent || !cg_shadowItems.integer || cg_simpleItems.integer)
	{
		return;
	}

	mapPoint = CG_ShadowItems_MapPointIndexForCent(cent);
	if (mapPoint < 0)
	{
		return;
	}

	CG_ShadowItems_RememberAnchorOrigin(mapPoint, cent->lerpOrigin);
	CG_ShadowItems_BindBobFromCent(mapPoint, cent);
}

int CG_ShadowItems_NumPoints(void)
{
	return cg_shadowItemCount;
}

int CG_ShadowItems_NumPointsForMode(int mode)
{
	if (mode < 0 || mode >= CG_SHADOW_MODE_COUNT)
	{
		return 0;
	}
	return cg_shadowModeCount[mode];
}

int CG_ShadowItems_ActiveModeIndex(void)
{
	return CG_ShadowItems_ModeIndex();
}

int CG_ShadowItems_DebugEffectiveMask(void)
{
	return CG_ShadowItems_EffectiveMask();
}

void CG_ShadowItems_PrintList(int maxList)
{
	int i;
	int n;

	if (!cg_shadowItemCount)
	{
		CG_Printf("  ^1no shadow item points loaded^7\n");
		return;
	}

	n = cg_shadowItemCount;
	if (maxList > 0 && maxList < n)
	{
		n = maxList;
	}

	CG_Printf("  ^3shadow items^7 (%i):\n", cg_shadowItemCount);
	for (i = 0; i < n; i++)
	{
		const cg_shadowItemPoint_t* pt;

		pt = &cg_shadowItemPoints[i];
		CG_Printf("  [%i] mi=%i cat=%i origin %.0f %.0f %.0f\n",
		          i, pt->modelindex, pt->shadowCat,
		          pt->origin[0], pt->origin[1], pt->origin[2]);
	}
	if (n < cg_shadowItemCount)
	{
		CG_Printf("  ... (%i more, use testq3lb list items <count>)\n", cg_shadowItemCount - n);
	}
}
