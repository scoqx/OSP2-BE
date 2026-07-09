/*
===========================================================================
cg_shadowitems.c

Ghost 3D pickups when the real item is absent (cg_shadowItems bitmask).
Data: server CS 890-899 for shadow coords when present (authoritative); QITM fallback when no server catalog. Requires cg_simpleItems 0.

Copyright (c) 2026 diwoc
===========================================================================
*/

#include "cg_local.h"
#include "../qcommon/qcommon.h"

#define CG_SHADOW_ITEM_MAX          384
#define CG_SHADOW_MODE_COUNT        5
#define CG_SHADOW_MATCH_XY_SQ       (4.0f * 4.0f)
#define CG_SHADOW_MATCH_Z           48.0f
/* Map team slot / SI3 group — not proximity merge. */
#define CG_SHADOW_QITM1_VERSION     3
#define CG_SHADOW_QITM1_VERSION_V2  2
#define CG_SHADOW_SERVER_PAYLOAD_SIZE (MAX_STRING_CHARS * CS_OSP2BE_ITEM_CATALOG_COUNT + 4)
/* Teamed map items respawn at random on server — cycle ghost preview on pad. */
#define CG_SHADOW_SLOT_ROTATE_MS    2000
#define CG_SHADOW_SLOT_CROSSFADE_MS 400
#define CG_SHADOW_GHOST_ALPHA       80
#define CG_SHADOW_PICKUP_MATCH_SQ   (96.0f * 96.0f)
#define CG_SHADOW_SERVER_MATCH_XY_SQ (64.0f * 64.0f)

#define CG_SHADOW_ITEM_DROP_DEPTH   4096.0f
#define CG_SHADOW_QITM_FLAG_SUSPENDED 1
#define CG_SHADOW_ITEM_NUDGE_STEP   8.0f
#define CG_SHADOW_ITEM_NUDGE_MAX    512.0f
#define CG_SHADOW_ITEM_MAX_DROP_DELTA 256.0f
/* Wider bind + anchor origin for armor/mega when map CM != server item (e.g. fatal). */
#define CG_SHADOW_ARMOR_MATCH_XY_SQ   (24.0f * 24.0f)

#define CG_SHADOW_ITEM_SLOT_MAX         128
#define CG_SHADOW_ITEM_SLOT_MEMBERS     16

typedef struct
{
	vec3_t  origin;
	int     modelindex;
	int     shadowCat;
	int     teamSlotKey;
} cg_shadowItemPoint_t;

static cg_shadowItemPoint_t     cg_shadowItemPoints[CG_SHADOW_ITEM_MAX];
static int                      cg_shadowItemCount;
static qboolean                 cg_shadowItemsFromServer;
static int                      cg_shadowModeIndices[CG_SHADOW_MODE_COUNT][CG_SHADOW_ITEM_MAX];
static int                      cg_shadowModeCount[CG_SHADOW_MODE_COUNT];
static qboolean                 cg_shadowPointActive[CG_SHADOW_MODE_COUNT][CG_SHADOW_ITEM_MAX];

typedef struct
{
	const char* abbrev;
	const char* classname;
} cg_shadowItemAbbrev_t;

static const cg_shadowItemAbbrev_t cg_shadowItemAbbrevTable[] =
{
	{ "h5", "item_health_small" },
	{ "h25", "item_health" },
	{ "h50", "item_health_large" },
	{ "mh", "item_health_mega" },
	{ "shd", "item_armor_shard" },
	{ "ya", "item_armor_combat" }, // yellow :50
	{ "ra", "item_armor_body" }, // red :100
	{ "ga", "item_armor_jacket" }, // green :25
	{ "gt", "weapon_gauntlet" },
	{ "sg", "weapon_shotgun" },
	{ "mg", "weapon_machinegun" },
	{ "gl", "weapon_grenadelauncher" },
	{ "rl", "weapon_rocketlauncher" },
	{ "lg", "weapon_lightning" },
	{ "rg", "weapon_railgun" },
	{ "pg", "weapon_plasmagun" },
	{ "bfg", "weapon_bfg" },
	{ "gh", "weapon_grapplinghook" },
	{ "sga", "ammo_shells" },
	{ "mga", "ammo_bullets" },
	{ "gla", "ammo_grenades" },
	{ "pga", "ammo_cells" },
	{ "lga", "ammo_lightning" },
	{ "rla", "ammo_rockets" },
	{ "rga", "ammo_slugs" },
	{ "bfa", "ammo_bfg" },
	{ "qd", "item_quad" },
	{ "bs", "item_enviro" },
	{ "hst", "item_haste" },
	{ "inv", "item_invis" },
	{ "reg", "item_regen" },
	{ "flg", "item_flight" },
	{ "med", "holdable_medkit" },
	{ "tp", "holdable_teleporter" },
	{ "rfl", "team_CTF_redflag" },
	{ "bfl", "team_CTF_blueflag" },
	{ "ng", "weapon_nailgun" },
	{ "pl", "weapon_prox_launcher" },
	{ "cg", "weapon_chaingun" },
	{ "na", "ammo_nails" },
	{ "mn", "ammo_mines" },
	{ "ab", "ammo_belt" },
	{ "kk", "holdable_kamikaze" },
	{ "pt", "holdable_portal" },
	{ "hv", "holdable_invulnerability" },
	{ "sc", "item_scout" },
	{ "gd", "item_guard" },
	{ "db", "item_doubler" },
	{ "ar", "item_ammoregen" },
	{ "rc", "item_redcube" },
	{ "bc", "item_bluecube" },
	{ "nf", "team_CTF_neutralflag" },
	{ NULL, NULL }
};

qboolean CG_ShadowItems_ClassnameFromAbbrev(const char* abbrev, const char** outClassname)
{
	const cg_shadowItemAbbrev_t* row;

	if (!abbrev || !abbrev[0] || !outClassname)
	{
		return qfalse;
	}

	for (row = cg_shadowItemAbbrevTable; row->abbrev; row++)
	{
		if (!Q_stricmp(abbrev, row->abbrev))
		{
			*outClassname = row->classname;
			return qtrue;
		}
	}

	return qfalse;
}

static int CG_ShadowItems_EffectiveMask(void)
{
	int cvarVal;

	cvarVal = cg_shadowItems.integer;
	if (!cvarVal)
	{
		return 0;
	}
	return cvarVal & CG_SHADOW_ITEMS_CATEGORY_MASK;
}

static int CG_ShadowItems_CatFromModelindex(int modelindex)
{
	gitem_t*    item;
	int         mask;

	if (modelindex < 1 || modelindex >= bg_numItems)
	{
		return 0;
	}
	item = &bg_itemlist[modelindex];
	mask = 0;
	if (item->giType == IT_HEALTH)
	{
		mask = CG_SHADOW_ITEMS_CAT_HEALTH;
	}
	else if (item->giType == IT_ARMOR)
	{
		mask = CG_SHADOW_ITEMS_CAT_ARMOR;
	}
	else if (item->giType == IT_WEAPON)
	{
		mask = CG_SHADOW_ITEMS_CAT_WEAPON;
	}
	else if (item->giType == IT_AMMO)
	{
		mask = CG_SHADOW_ITEMS_CAT_AMMO;
	}
	else if (item->giType == IT_POWERUP || item->giType == IT_HOLDABLE
	         || item->giType == IT_PERSISTANT_POWERUP)
	{
		mask = CG_SHADOW_ITEMS_CAT_POWERUP;
	}
	else if (item->giType == IT_TEAM)
	{
		mask = CG_SHADOW_ITEMS_CAT_FLAG;
	}
	return mask;
}

static int CG_ShadowItems_ModelindexFromToken(const char* tok)
{
	const cg_shadowItemAbbrev_t* row;
	gitem_t*    item;
	int         i;
	int         n;

	if (!tok || !tok[0])
	{
		return 0;
	}
	if (tok[0] == 'i')
	{
		n = atoi(tok + 1);
		if (n >= 1 && n < bg_numItems)
		{
			return n;
		}
		return 0;
	}
	for (row = cg_shadowItemAbbrevTable; row->abbrev; row++)
	{
		if (!Q_stricmp(tok, row->abbrev))
		{
			for (i = 1; i < bg_numItems; i++)
			{
				item = &bg_itemlist[i];
				if (!Q_stricmp(item->classname, row->classname))
				{
					return i;
				}
			}
			return 0;
		}
	}
	n = atoi(tok);
	if (n >= 1 && n < bg_numItems)
	{
		return n;
	}
	return 0;
}

static qboolean CG_ShadowItems_AddServerPoint(int modelindex, int ix, int iy, int iz, int teamSlotKey)
{
	cg_shadowItemPoint_t* pt;

	if (modelindex <= 0 || cg_shadowItemCount >= CG_SHADOW_ITEM_MAX)
	{
		return qfalse;
	}
	pt = &cg_shadowItemPoints[cg_shadowItemCount];
	pt->origin[0] = ix * 0.1f;
	pt->origin[1] = iy * 0.1f;
	pt->origin[2] = iz * 0.1f;
	pt->modelindex = modelindex;
	pt->shadowCat = CG_ShadowItems_CatFromModelindex(modelindex);
	pt->teamSlotKey = teamSlotKey;
	cg_shadowItemCount++;
	return qtrue;
}

static qboolean CG_ShadowItems_ParseServerGroup(char* groupStr, int teamSlotKey)
{
	char*       at;
	char*       itemTok;
	char*       slash;
	int         ix;
	int         iy;
	int         iz;
	int         modelindex;

	at = Q_strrchr(groupStr, ';');
	if (!at)
	{
		return qfalse;
	}
	*at = '\0';
	at++;
	if (Q_sscanf(at, "%d,%d,%d", &ix, &iy, &iz) != 3)
	{
		return qfalse;
	}

	itemTok = groupStr;
	while (itemTok && itemTok[0])
	{
		slash = strchr(itemTok, '/');
		if (slash)
		{
			*slash = '\0';
		}
		modelindex = CG_ShadowItems_ModelindexFromToken(itemTok);
		if (modelindex > 0)
		{
			CG_ShadowItems_AddServerPoint(modelindex, ix, iy, iz, teamSlotKey);
		}
		if (slash)
		{
			itemTok = slash + 1;
		}
		else
		{
			itemTok = NULL;
		}
	}
	return qtrue;
}

static void CG_ShadowItems_MarkAllPointsActive(void)
{
	int     mode;
	int     i;

	for (mode = 0; mode < CG_SHADOW_MODE_COUNT; mode++)
	{
		cg_shadowModeCount[mode] = cg_shadowItemCount;
		for (i = 0; i < cg_shadowItemCount; i++)
		{
			cg_shadowModeIndices[mode][i] = i;
			cg_shadowPointActive[mode][i] = qtrue;
		}
	}
}

static void CG_ShadowItems_Clear(void);
static void CG_ShadowItems_BuildPickupSlots(void);
static void CG_ShadowItems_BuildModelBuckets(void);
static void CG_ShadowItems_RebuildTrackSlotMap(void);

static qboolean CG_ShadowItems_TryLoadCatalogConfigstrings(void)
{
	char            payload[CG_SHADOW_SERVER_PAYLOAD_SIZE];
	const char*     chunk;
	char*           seg;
	char*           nextSeg;
	int             i;
	int             len;
	int             serverSlotKey;

	payload[0] = '\0';
	len = 0;

	for (i = 0; i < CS_OSP2BE_ITEM_CATALOG_COUNT; i++)
	{
		chunk = CG_ConfigString(CS_OSP2BE_ITEM_CATALOG0 + i);
		if (!chunk[0])
		{
			continue;
		}
		if (len > 0 && len + (int)strlen(chunk) >= (int)sizeof(payload) - 1)
		{
			CG_Printf(S_COLOR_RED "ERROR: item catalog client payload overflow\n");
			break;
		}
		Q_strcat(payload, sizeof(payload), chunk);
		len = (int)strlen(payload);
	}

	if (len <= 0)
	{
		return qfalse;
	}

	{
		int     catalogMode;

		catalogMode = 1;
		if (payload[0] >= '1' && payload[0] <= '3')
		{
			catalogMode = payload[0] - '0';
		}
		if (!(catalogMode & 1))
		{
			return qfalse;
		}
	}

	CG_ShadowItems_Clear();

	seg = payload;
	if (seg[0] >= '1' && seg[0] <= '3')
	{
		seg++;
	}
	serverSlotKey = 1;
	while (seg && seg[0])
	{
		nextSeg = strchr(seg, '|');
		if (nextSeg)
		{
			*nextSeg = '\0';
			nextSeg++;
		}
		CG_ShadowItems_ParseServerGroup(seg, serverSlotKey);
		serverSlotKey++;
		seg = nextSeg;
	}

	if (cg_shadowItemCount <= 0)
	{
		CG_ShadowItems_Clear();
		return qfalse;
	}

	return qtrue;
}

static int                      cg_shadowItemSlotId[CG_SHADOW_ITEM_MAX];
static int                      cg_shadowItemSlotMembers[CG_SHADOW_ITEM_SLOT_MAX][CG_SHADOW_ITEM_SLOT_MEMBERS];
static int                      cg_shadowItemSlotMemberCount[CG_SHADOW_ITEM_SLOT_MAX];
static int                      cg_shadowItemSlotCount;
static qboolean                 cg_shadowItemVisibleDraw[CG_SHADOW_ITEM_MAX];
static int                      cg_shadowModelPointStart[MAX_ITEMS];
static int                      cg_shadowModelPointCount[MAX_ITEMS];
static int                      cg_shadowModelPointIdx[CG_SHADOW_ITEM_MAX];
static float                    cg_shadowMatchXySqByModel[MAX_ITEMS];
static qboolean                 cg_shadowUsesAnchorOriginByModel[MAX_ITEMS];
static qboolean                 cg_shadowItemSlotVisible[CG_SHADOW_ITEM_SLOT_MAX];
static vec3_t                   cg_shadowSlotDrawOrigin[CG_SHADOW_ITEM_SLOT_MAX];
static int                      cg_shadowMatchBestEnt[CG_SHADOW_ITEM_MAX];
static float                    cg_shadowMatchBestAbsDz[CG_SHADOW_ITEM_MAX];
/* Item bob / scale-up phase per pickup pad (slot), not per QITM point. */
static int                      cg_shadowSlotBobMiscTime[CG_SHADOW_ITEM_SLOT_MAX];
/* Multi-item slot ghost rotation: start at picked item, phase from pickup time. */
static int                      cg_shadowSlotRotateBaseTime[CG_SHADOW_ITEM_SLOT_MAX];
static int                      cg_shadowSlotRotateLead[CG_SHADOW_ITEM_SLOT_MAX];
static qboolean                 cg_shadowSlotServerSync[CG_SHADOW_ITEM_SLOT_MAX];
static int                      cg_shadowTrackSlot[CG_ITEM_NOTIFY_MAX_ENTRIES];
/* Last seen live item origin (armor/mega); ghost stays here after pickup. */
static vec3_t                   cg_shadowItemAnchorOrigin[CG_SHADOW_ITEM_MAX];
static qboolean                 cg_shadowItemAnchorValid[CG_SHADOW_ITEM_MAX];
static int                      cg_shadowFrameSerial;
static int                      cg_shadowFrameMask;
static int                      cg_shadowFrameMode;
static qboolean                 cg_shadowFrameActive;

static qboolean CG_ShadowItems_GameAllows(void)
{
	if (CG_OSPIsGameTypeCA(cgs.gametype))
	{
		return qfalse;
	}
	return qtrue;
}

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
	memset(cg_shadowItemSlotId, 0xff, sizeof(cg_shadowItemSlotId));
	cg_shadowItemSlotCount = 0;
	memset(cg_shadowItemSlotMemberCount, 0, sizeof(cg_shadowItemSlotMemberCount));
	memset(cg_shadowPointActive, 0, sizeof(cg_shadowPointActive));
	memset(cg_shadowModelPointStart, 0, sizeof(cg_shadowModelPointStart));
	memset(cg_shadowModelPointCount, 0, sizeof(cg_shadowModelPointCount));
	memset(cg_shadowSlotBobMiscTime, 0, sizeof(cg_shadowSlotBobMiscTime));
	memset(cg_shadowSlotRotateBaseTime, 0, sizeof(cg_shadowSlotRotateBaseTime));
	memset(cg_shadowSlotRotateLead, 0, sizeof(cg_shadowSlotRotateLead));
	memset(cg_shadowSlotServerSync, 0, sizeof(cg_shadowSlotServerSync));
	memset(cg_shadowTrackSlot, 0xff, sizeof(cg_shadowTrackSlot));
	memset(cg_shadowItemAnchorValid, 0, sizeof(cg_shadowItemAnchorValid));
}

static void CG_ShadowItems_BuildModelMatchCache(void)
{
	int         mi;
	gitem_t*    item;

	for (mi = 0; mi < MAX_ITEMS; mi++)
	{
		cg_shadowUsesAnchorOriginByModel[mi] = qfalse;
		cg_shadowMatchXySqByModel[mi] = CG_SHADOW_MATCH_XY_SQ;
	}

	for (mi = 1; mi < bg_numItems; mi++)
	{
		item = &bg_itemlist[mi];
		if (!Q_stricmp(item->classname, "item_health_mega"))
		{
			cg_shadowUsesAnchorOriginByModel[mi] = qtrue;
			cg_shadowMatchXySqByModel[mi] = CG_SHADOW_ARMOR_MATCH_XY_SQ;
		}
		else if (item->giType == IT_ARMOR && Q_stricmp(item->classname, "item_armor_shard"))
		{
			cg_shadowUsesAnchorOriginByModel[mi] = qtrue;
			cg_shadowMatchXySqByModel[mi] = CG_SHADOW_ARMOR_MATCH_XY_SQ;
		}
	}
}

static void CG_ShadowItems_RememberAnchorOrigin(int pointIdx, const vec3_t origin)
{
	const cg_shadowItemPoint_t* pt;
	int         mi;

	if (pointIdx < 0 || pointIdx >= CG_SHADOW_ITEM_MAX)
	{
		return;
	}
	pt = &cg_shadowItemPoints[pointIdx];
	mi = pt->modelindex;
	if (mi < 0 || mi >= MAX_ITEMS || !cg_shadowUsesAnchorOriginByModel[mi])
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
	int                     pointRecSize;
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
	if (version != CG_SHADOW_QITM1_VERSION && version != CG_SHADOW_QITM1_VERSION_V2)
	{
		return qfalse;
	}

	pointRecSize = (version >= CG_SHADOW_QITM1_VERSION) ? 18 : 16;

	pos = 9;
	CG_ShadowItems_Clear();

	for (i = 0; i < numPoints; i++)
	{
		if (pos + pointRecSize > rawLen)
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
		if (version >= CG_SHADOW_QITM1_VERSION)
		{
			pt->teamSlotKey = CG_ShadowItems_ReadLE16(raw + pos);
			pos += 2;
		}
		else
		{
			pt->teamSlotKey = i + 1;
		}
		CG_ShadowItems_DropToFloor(spawn, suspended, pt->origin);
	}
	cg_shadowItemCount = numPoints;

	if (!CG_ShadowItems_LoadQITM1Modes(raw, rawLen, pos))
	{
		return qfalse;
	}

	return qtrue;
}

/*
==================
CG_ShadowItems_EnrichTeamSlotKeysFromQITM

Server catalog carries no team-slot grouping; borrow it from the packed QITM
layout by XY proximity. LoadQITM1 overwrites the point array, so the catalog is
snapshotted first and restored afterwards (with the enriched keys).
==================
*/
static void CG_ShadowItems_EnrichTeamSlotKeysFromQITM(const byte* qitm, int qitmLen)
{
	cg_shadowItemPoint_t    catalogCopy[CG_SHADOW_ITEM_MAX];
	int                     catalogCopyCount;
	int                     qitmCount;
	int                     i;
	int                     j;
	int                     bestKey;
	float                   dx;
	float                   dy;
	float                   distSq;
	float                   bestDistSq;

	catalogCopyCount = cg_shadowItemCount;
	if (catalogCopyCount > CG_SHADOW_ITEM_MAX)
	{
		catalogCopyCount = CG_SHADOW_ITEM_MAX;
	}
	memcpy(catalogCopy, cg_shadowItemPoints, catalogCopyCount * sizeof(catalogCopy[0]));

	if (!CG_ShadowItems_LoadQITM1(qitm, qitmLen))
	{
		memcpy(cg_shadowItemPoints, catalogCopy, catalogCopyCount * sizeof(catalogCopy[0]));
		cg_shadowItemCount = catalogCopyCount;
		return;
	}

	qitmCount = cg_shadowItemCount;
	for (i = 0; i < catalogCopyCount; i++)
	{
		bestKey = 0;
		bestDistSq = CG_SHADOW_SERVER_MATCH_XY_SQ + 1.0f;
		for (j = 0; j < qitmCount; j++)
		{
			dx = catalogCopy[i].origin[0] - cg_shadowItemPoints[j].origin[0];
			dy = catalogCopy[i].origin[1] - cg_shadowItemPoints[j].origin[1];
			distSq = dx * dx + dy * dy;
			if (distSq < bestDistSq)
			{
				bestDistSq = distSq;
				bestKey = cg_shadowItemPoints[j].teamSlotKey;
			}
		}
		if (bestKey > 0 && bestDistSq <= CG_SHADOW_SERVER_MATCH_XY_SQ)
		{
			catalogCopy[i].teamSlotKey = bestKey;
		}
	}

	memcpy(cg_shadowItemPoints, catalogCopy, catalogCopyCount * sizeof(catalogCopy[0]));
	cg_shadowItemCount = catalogCopyCount;
}

/*
==================
CG_ShadowItems_Reload

Single pipeline: fill points from one source (server catalog is authoritative,
QITM bundle is the fallback), then build the shared derived structures. The only
per-source differences are the origin of the points and the cg_shadowItemsFromServer
flag (which drives anchor/armor handling in BuildModelBuckets and server rotation).
==================
*/
static void CG_ShadowItems_Reload(void)
{
	const byte*     qitm;
	int             qitmLen;
	qboolean        haveQitm;

	haveQitm = CG_MapBundle_Section("QITM", &qitm, &qitmLen);

	if (CG_ShadowItems_TryLoadCatalogConfigstrings())
	{
		cg_shadowItemsFromServer = qtrue;
		if (haveQitm)
		{
			CG_ShadowItems_EnrichTeamSlotKeysFromQITM(qitm, qitmLen);
		}
		CG_ShadowItems_MarkAllPointsActive();
	}
	else
	{
		cg_shadowItemsFromServer = qfalse;
		if (!haveQitm || !CG_ShadowItems_LoadQITM1(qitm, qitmLen))
		{
			CG_ShadowItems_Clear();
			return;
		}
	}

	CG_ShadowItems_BuildPickupSlots();
	CG_ShadowItems_BuildModelBuckets();
	CG_ShadowItems_RebuildTrackSlotMap();
}

static int CG_ShadowItems_FindSlotByPadOrigin(const vec3_t padOrigin)
{
	int     slot;
	int     s;
	float   bestDistSq;
	float   dx;
	float   dy;
	float   distSq;

	slot = -1;
	bestDistSq = CG_SHADOW_SERVER_MATCH_XY_SQ + 1.0f;
	for (s = 0; s < cg_shadowItemSlotCount; s++)
	{
		dx = padOrigin[0] - cg_shadowSlotDrawOrigin[s][0];
		dy = padOrigin[1] - cg_shadowSlotDrawOrigin[s][1];
		distSq = dx * dx + dy * dy;
		if (distSq > CG_SHADOW_SERVER_MATCH_XY_SQ)
		{
			continue;
		}
		if (slot < 0 || distSq < bestDistSq)
		{
			bestDistSq = distSq;
			slot = s;
		}
	}
	return slot;
}

static void CG_ShadowItems_RebuildTrackSlotMap(void)
{
	int     trackId;
	int     ix;
	int     iy;
	int     iz;
	int     slot;
	vec3_t  padOrigin;

	memset(cg_shadowTrackSlot, 0xff, sizeof(cg_shadowTrackSlot));
	if (!cg_shadowItemSlotCount)
	{
		return;
	}

	for (trackId = 0; trackId < CG_ITEM_NOTIFY_MAX_ENTRIES; trackId++)
	{
		if (!CG_SpectatorItems_GetTrackPadCoords(trackId, &ix, &iy, &iz))
		{
			continue;
		}
		padOrigin[0] = ix * 0.1f;
		padOrigin[1] = iy * 0.1f;
		padOrigin[2] = iz * 0.1f;
		slot = CG_ShadowItems_FindSlotByPadOrigin(padOrigin);
		if (slot >= 0)
		{
			cg_shadowTrackSlot[trackId] = slot;
		}
	}
}

void CG_ShadowItems_Load(void)
{
	CG_ShadowItems_Reload();
}

void CG_ShadowItems_OnConfigStringChanged(void)
{
	CG_ShadowItems_Reload();
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

static qboolean CG_ShadowItems_CanSharePickupSlot(
        const cg_shadowItemPoint_t* a,
        const cg_shadowItemPoint_t* b)
{
	if (a->modelindex < 1 || a->modelindex >= bg_numItems
	        || b->modelindex < 1 || b->modelindex >= bg_numItems)
	{
		return qfalse;
	}
	if (!Q_stricmp(bg_itemlist[a->modelindex].classname, "item_health")
	        || !Q_stricmp(bg_itemlist[b->modelindex].classname, "item_health"))
	{
		return qfalse;
	}
	if (a->teamSlotKey <= 0 || b->teamSlotKey <= 0)
	{
		return qfalse;
	}
	return (a->teamSlotKey == b->teamSlotKey);
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

/*
visibleDraw: 1 = live item, 0 = EF_NODRAW sync, -1 = anchor/bob only (respawn)
*/
static int CG_ShadowItems_MatchCent(centity_t* cent, int mask, int visibleDraw)
{
	int         j;
	int         idx;
	int         mi;
	int         start;
	int         end;
	int         bestEnt;
	int         bestPoint;
	int         slot;
	float       dz;
	float       absDz;
	float       bestPointDz;
	float       dx;
	float       dy;
	float       xySq;
	const float* world;
	const cg_shadowItemPoint_t* pt;
	const float* mapOrigin;

	if (!cent || !cent->currentValid || cent->currentState.eType != ET_ITEM)
	{
		return -1;
	}

	mi = cent->currentState.modelindex;
	if (mi < 0 || mi >= MAX_ITEMS || !cg_shadowModelPointCount[mi])
	{
		return -1;
	}

	world = cent->lerpOrigin;
	start = cg_shadowModelPointStart[mi];
	end = start + cg_shadowModelPointCount[mi];
	xySq = cg_shadowMatchXySqByModel[mi];
	bestPoint = -1;
	bestPointDz = CG_SHADOW_MATCH_Z + 1.0f;

	for (j = start; j < end; j++)
	{
		idx = cg_shadowModelPointIdx[j];
		if (idx < 0 || idx >= CG_SHADOW_ITEM_MAX)
		{
			continue;
		}
		pt = &cg_shadowItemPoints[idx];
		if (!(mask & pt->shadowCat))
		{
			continue;
		}

		mapOrigin = pt->origin;
		dx = world[0] - mapOrigin[0];
		dy = world[1] - mapOrigin[1];
		if (dx * dx + dy * dy > xySq)
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

		if (visibleDraw >= 0)
		{
			cg_shadowMatchBestAbsDz[idx] = absDz;
			cg_shadowMatchBestEnt[idx] = cent->currentState.number;
			cg_shadowItemVisibleDraw[idx] = (visibleDraw > 0) ? qtrue : qfalse;
		}

		CG_ShadowItems_RememberAnchorOrigin(idx, cent->lerpOrigin);
		slot = cg_shadowItemSlotId[idx];
		if (slot >= 0 && slot < CG_SHADOW_ITEM_SLOT_MAX)
		{
			if (visibleDraw > 0)
			{
				cg_shadowSlotBobMiscTime[slot] = cent->miscTime;
				cg_shadowItemSlotVisible[slot] = qtrue;
				cg_shadowSlotServerSync[slot] = qfalse;
			}
			else if (visibleDraw == 0 || visibleDraw < 0)
			{
				cg_shadowSlotBobMiscTime[slot] = cent->miscTime;
			}
		}

		if (bestPoint < 0 || absDz < bestPointDz
		        || (absDz == bestPointDz && idx < bestPoint))
		{
			bestPoint = idx;
			bestPointDz = absDz;
		}
	}

	return bestPoint;
}

static int CG_ShadowItems_GhostAlpha(int pointIdx, int mode, int mask)
{
	int     slot;
	int     n;
	int     nActive;
	int     k;
	int     idx;
	int     lead;
	int     myShowIdx;
	int     elapsed;
	int     phaseMs;
	int     tick;
	int     fadePos;
	int     currentShow;
	int     nextShow;
	int     alpha;
	int     crossStart;
	float   fade;
	const cg_shadowItemPoint_t* pt;

	if (pointIdx < 0 || pointIdx >= cg_shadowItemCount
	        || !cg_shadowPointActive[mode][pointIdx])
	{
		return 0;
	}

	pt = &cg_shadowItemPoints[pointIdx];
	if (!(mask & pt->shadowCat) || cg_shadowItemVisibleDraw[pointIdx])
	{
		return 0;
	}

	slot = cg_shadowItemSlotId[pointIdx];
	if (slot >= 0 && slot < CG_SHADOW_ITEM_SLOT_MAX && cg_shadowItemSlotVisible[slot])
	{
		return 0;
	}

	n = (slot >= 0 && slot < CG_SHADOW_ITEM_SLOT_MAX)
	    ? cg_shadowItemSlotMemberCount[slot] : 1;
	if (n <= 1)
	{
		return CG_SHADOW_GHOST_ALPHA;
	}

	nActive = 0;
	myShowIdx = -1;
	for (k = 0; k < n; k++)
	{
		idx = cg_shadowItemSlotMembers[slot][k];
		if (!cg_shadowPointActive[mode][idx])
		{
			continue;
		}
		if (!(mask & cg_shadowItemPoints[idx].shadowCat))
		{
			continue;
		}
		if (cg_shadowItemVisibleDraw[idx])
		{
			continue;
		}
		if (idx == pointIdx)
		{
			myShowIdx = nActive;
		}
		nActive++;
	}

	if (nActive <= 0 || myShowIdx < 0)
	{
		return 0;
	}
	if (nActive == 1)
	{
		return CG_SHADOW_GHOST_ALPHA;
	}

	if (cg_shadowSlotServerSync[slot])
	{
		lead = cg_shadowSlotRotateLead[slot];
		if (lead < 0 || lead >= nActive)
		{
			lead = 0;
		}
		return (myShowIdx == lead) ? CG_SHADOW_GHOST_ALPHA : 0;
	}

	lead = cg_shadowSlotRotateLead[slot];
	if (lead < 0 || lead >= nActive)
	{
		lead = 0;
	}

	elapsed = cg.time - cg_shadowSlotRotateBaseTime[slot];
	if (elapsed < 0)
	{
		elapsed = 0;
	}

	phaseMs = elapsed % (nActive * CG_SHADOW_SLOT_ROTATE_MS);
	tick = phaseMs / CG_SHADOW_SLOT_ROTATE_MS;
	fadePos = phaseMs % CG_SHADOW_SLOT_ROTATE_MS;
	currentShow = (lead + tick) % nActive;
	nextShow = (lead + tick + 1) % nActive;
	crossStart = CG_SHADOW_SLOT_ROTATE_MS - CG_SHADOW_SLOT_CROSSFADE_MS;
	alpha = 0;

	if (myShowIdx == currentShow)
	{
		if (fadePos < crossStart || CG_SHADOW_SLOT_CROSSFADE_MS <= 0)
		{
			alpha = CG_SHADOW_GHOST_ALPHA;
		}
		else
		{
			fade = (float)(fadePos - crossStart) / (float)CG_SHADOW_SLOT_CROSSFADE_MS;
			alpha = (int)((float)CG_SHADOW_GHOST_ALPHA * (1.0f - fade));
		}
	}
	else if (myShowIdx == nextShow && CG_SHADOW_SLOT_CROSSFADE_MS > 0 && fadePos >= crossStart)
	{
		fade = (float)(fadePos - crossStart) / (float)CG_SHADOW_SLOT_CROSSFADE_MS;
		alpha = (int)((float)CG_SHADOW_GHOST_ALPHA * fade);
	}

	return alpha;
}

static void CG_ShadowItems_BuildModelBuckets(void)
{
	int     counts[MAX_ITEMS];
	int     i;
	int     mi;
	int     pos;

	CG_ShadowItems_BuildModelMatchCache();

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

	if (cg_shadowItemsFromServer)
	{
		for (mi = 0; mi < MAX_ITEMS; mi++)
		{
			cg_shadowUsesAnchorOriginByModel[mi] = qfalse;
			cg_shadowMatchXySqByModel[mi] = CG_SHADOW_MATCH_XY_SQ;
		}
	}
}


static void CG_ShadowItems_BeginFrame(void)
{
	int     k;

	if (cg_shadowFrameSerial == cg.clientFrame)
	{
		return;
	}

	cg_shadowFrameSerial = cg.clientFrame;
	cg_shadowFrameActive = qfalse;
	cg_shadowFrameMask = 0;

	if (cg_simpleItems.integer || !cg_shadowItems.integer || !cg_shadowItemCount || !cg.snap
	        || !CG_ShadowItems_GameAllows())
	{
		return;
	}

	cg_shadowFrameMask = CG_ShadowItems_EffectiveMask();
	if (!cg_shadowFrameMask)
	{
		return;
	}

	cg_shadowFrameMode = CG_ShadowItems_ModeIndex();
	if (cg_shadowFrameMode < 0 || cg_shadowFrameMode >= CG_SHADOW_MODE_COUNT)
	{
		return;
	}

	cg_shadowFrameActive = qtrue;
	memset(cg_shadowItemSlotVisible, 0, sizeof(cg_shadowItemSlotVisible));
	for (k = 0; k < cg_shadowItemCount; k++)
	{
		cg_shadowItemVisibleDraw[k] = qfalse;
		cg_shadowMatchBestEnt[k] = -1;
		cg_shadowMatchBestAbsDz[k] = CG_SHADOW_MATCH_Z + 1.0f;
	}
}

int CG_ShadowItems_NoteLiveItem(centity_t* cent)
{
	if (cg_simpleItems.integer || !cg_shadowItems.integer || !cg_shadowItemCount || !cent)
	{
		return -1;
	}

	CG_ShadowItems_BeginFrame();

	if (!cg_shadowFrameActive)
	{
		return -1;
	}

	return CG_ShadowItems_MatchCent(cent, cg_shadowFrameMask, 1);
}

void CG_AddShadowItems(void)
{
	int         i;
	int         idx;
	int         slot;
	int         ghostAlpha;
	int         mode;
	int         mask;
	int         mi;
	int         bobTime;
	int         phaseEnt;
	centity_t*  cent;
	entityState_t* es;
	const cg_shadowItemPoint_t* pt;
	vec3_t      drawOrigin;

	CG_ShadowItems_BeginFrame();

	if (cg_simpleItems.integer || !cg_shadowFrameActive || !cg.snap)
	{
		return;
	}

	mode = cg_shadowFrameMode;
	mask = cg_shadowFrameMask;

	for (i = 0; i < cg.snap->numEntities; i++)
	{
		es = &cg.snap->entities[i];
		if (es->eType != ET_ITEM || !(es->eFlags & EF_NODRAW))
		{
			continue;
		}
		cent = &cg_entities[es->number];
		CG_ShadowItems_MatchCent(cent, mask, 0);
	}

	for (i = 0; i < cg_shadowModeCount[mode]; i++)
	{
		idx = cg_shadowModeIndices[mode][i];
		if (idx < 0 || idx >= cg_shadowItemCount)
		{
			continue;
		}
		pt = &cg_shadowItemPoints[idx];
		if (!(mask & pt->shadowCat) || cg_shadowItemVisibleDraw[idx])
		{
			continue;
		}

		slot = cg_shadowItemSlotId[idx];
		if (slot >= 0 && slot < CG_SHADOW_ITEM_SLOT_MAX && cg_shadowItemSlotVisible[slot])
		{
			continue;
		}

		ghostAlpha = CG_ShadowItems_GhostAlpha(idx, mode, mask);
		if (ghostAlpha <= 0)
		{
			continue;
		}

		mi = pt->modelindex;
		if (mi >= 0 && mi < MAX_ITEMS && cg_shadowUsesAnchorOriginByModel[mi]
		        && cg_shadowItemAnchorValid[idx])
		{
			VectorCopy(cg_shadowItemAnchorOrigin[idx], drawOrigin);
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

		if (slot >= 0 && slot < CG_SHADOW_ITEM_SLOT_MAX)
		{
			phaseEnt = MAX_CLIENTS + slot;
			bobTime = cg_shadowSlotBobMiscTime[slot];
			if (!bobTime)
			{
				bobTime = cg.time;
			}
		}
		else
		{
			phaseEnt = idx + MAX_CLIENTS;
			bobTime = cg.time;
		}

		CG_AddWorldItem3D(drawOrigin, pt->modelindex, phaseEnt, bobTime, qtrue, ghostAlpha);
	}
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

int CG_ShadowItems_MapPointForItemBob(centity_t* cent)
{
	if (!cent || cg_simpleItems.integer || !cg_shadowItemCount || !CG_ShadowItems_GameAllows())
	{
		return -1;
	}

	return CG_ShadowItems_MatchCent(cent, CG_SHADOW_ITEMS_CATEGORY_MASK, -1);
}

void CG_ShadowItems_OnServerNextItem(int modelindex, int trackId)
{
	int     mode;
	int     mask;
	int     slot;
	int     n;
	int     k;
	int     idx;
	int     ord;

	if (!cg_shadowItems.integer || cg_simpleItems.integer || !CG_ShadowItems_GameAllows())
	{
		return;
	}
	if (!cg_shadowItemsFromServer)
	{
		return;
	}
	if (modelindex < 1 || modelindex >= bg_numItems)
	{
		return;
	}
	if (trackId < 0 || trackId >= CG_ITEM_NOTIFY_MAX_ENTRIES || !cg_shadowItemCount)
	{
		return;
	}

	mode = CG_ShadowItems_ModeIndex();
	mask = CG_ShadowItems_EffectiveMask();
	if (!(mask & CG_ShadowItems_CatFromModelindex(modelindex)))
	{
		return;
	}

	slot = cg_shadowTrackSlot[trackId];
	if (slot < 0 || slot >= CG_SHADOW_ITEM_SLOT_MAX)
	{
		return;
	}

	n = cg_shadowItemSlotMemberCount[slot];
	ord = 0;
	for (k = 0; k < n; k++)
	{
		idx = cg_shadowItemSlotMembers[slot][k];
		if (!cg_shadowPointActive[mode][idx])
		{
			continue;
		}
		if (!(mask & cg_shadowItemPoints[idx].shadowCat))
		{
			continue;
		}
		if (cg_shadowItemPoints[idx].modelindex == modelindex)
		{
			cg_shadowSlotRotateLead[slot] = ord;
			cg_shadowSlotServerSync[slot] = qtrue;
			cg_shadowSlotRotateBaseTime[slot] = cg.time;
			cg_shadowSlotBobMiscTime[slot] = cg.time;
			return;
		}
		ord++;
	}
}

void CG_ShadowItems_OnItemPickup(int modelindex, const vec3_t playerOrigin)
{
	int     mode;
	int     mask;
	int     i;
	int     k;
	int     slot;
	int     idx;
	int     n;
	int     ord;
	int     bestPoint;
	float   bestDistSq;
	float   dx;
	float   dy;
	float   dz;
	float   distSq;

	if (!cg_shadowItems.integer || cg_simpleItems.integer || !CG_ShadowItems_GameAllows())
	{
		return;
	}
	if (modelindex < 1 || modelindex >= bg_numItems || !playerOrigin)
	{
		return;
	}

	mode = CG_ShadowItems_ModeIndex();
	mask = CG_ShadowItems_EffectiveMask();

	bestPoint = -1;
	bestDistSq = CG_SHADOW_PICKUP_MATCH_SQ + 1.0f;

	for (i = 0; i < cg_shadowItemCount; i++)
	{
		if (cg_shadowItemPoints[i].modelindex != modelindex)
		{
			continue;
		}
		slot = cg_shadowItemSlotId[i];
		n = (slot >= 0 && slot < CG_SHADOW_ITEM_SLOT_MAX)
		    ? cg_shadowItemSlotMemberCount[slot] : 0;
		if (slot < 0 || slot >= CG_SHADOW_ITEM_SLOT_MAX || n <= 1)
		{
			continue;
		}

		dx = playerOrigin[0] - cg_shadowSlotDrawOrigin[slot][0];
		dy = playerOrigin[1] - cg_shadowSlotDrawOrigin[slot][1];
		dz = playerOrigin[2] - cg_shadowSlotDrawOrigin[slot][2];
		distSq = dx * dx + dy * dy + dz * dz;
		if (distSq > CG_SHADOW_PICKUP_MATCH_SQ)
		{
			continue;
		}
		if (bestPoint < 0 || distSq < bestDistSq)
		{
			bestDistSq = distSq;
			bestPoint = i;
		}
	}

	if (bestPoint < 0)
	{
		return;
	}

	slot = cg_shadowItemSlotId[bestPoint];
	n = cg_shadowItemSlotMemberCount[slot];

	ord = 0;
	for (k = 0; k < n; k++)
	{
		idx = cg_shadowItemSlotMembers[slot][k];
		if (!cg_shadowPointActive[mode][idx])
		{
			continue;
		}
		if (!(mask & cg_shadowItemPoints[idx].shadowCat))
		{
			continue;
		}
		if (idx == bestPoint)
		{
			cg_shadowSlotRotateLead[slot] = ord;
			cg_shadowSlotServerSync[slot] = qfalse;
			cg_shadowSlotRotateBaseTime[slot] = cg.time;
			cg_shadowSlotBobMiscTime[slot] = cg.time;
			break;
		}
		ord++;
	}
}

void CG_ShadowItems_OnItemRespawnEvent(centity_t* cent)
{
	if (!cent || !cg_shadowItems.integer || cg_simpleItems.integer || !cg_shadowItemCount
	        || !CG_ShadowItems_GameAllows())
	{
		return;
	}

	(void)CG_ShadowItems_MatchCent(cent, CG_ShadowItems_EffectiveMask(), -1);
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

int CG_ShadowItems_UsesServerLayout(void)
{
	return cg_shadowItemsFromServer ? 1 : 0;
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
		CG_Printf("  [%i] mi=%i cat=%i tsk=%i origin %.0f %.0f %.0f\n",
		          i, pt->modelindex, pt->shadowCat, pt->teamSlotKey,
		          pt->origin[0], pt->origin[1], pt->origin[2]);
	}
	if (n < cg_shadowItemCount)
	{
		CG_Printf("  ... (%i more, use testq3lb list items <count>)\n", cg_shadowItemCount - n);
	}
}
