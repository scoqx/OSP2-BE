/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.
===========================================================================
*/

/*
 * Spectator item pickup / spawn HUD (CS 890-899 PUI catalog + `pui` cmd).
 *
 * The server (metarena g_items_catalogue.c) already does the heavy lifting:
 * it classifies CTF sides, groups co-located items into a single segment
 * (abbrevs joined by '/'), and assigns trackId left-to-right across the
 * emitted abbrevs. So the client just mirrors that: one PUI segment == one
 * spot, side is taken from the wire, and trackId numbering follows the same
 * left-to-right order. No client-side flag geometry or pad reconstruction.
 */

#include "cg_local.h"
#include "superhud.h"
#include "../qcommon/qcommon.h"

typedef struct
{
	qboolean active;
	int itemId;
	int group;
	int nextSpawnTime;
	int trackSide;
	int csOrder;
	int padIx;
	int padIy;
	int padIz;
	qboolean havePadCoords;
} cg_spectatorItemEntry_t;

typedef struct
{
	int side;
	int csOrder;
	int memberCount;
	int members[CG_ITEM_SPAWNS_SPOT_MEMBERS_MAX];
} cg_spectatorItemSpot_t;

static cg_spectatorItemEntry_t cg_spectatorItems[CG_ITEM_NOTIFY_MAX_ENTRIES];
static cg_spectatorItemSpot_t cg_spectatorItemSpots[CG_ITEM_SPAWNS_SPOT_MAX];
static int cg_spectatorItemSpotCount;
static qboolean cg_spectatorItemsHudRedTable;
static qboolean cg_spectatorItemsHudBlueTable;

void CG_SpectatorItems_SetHudTeamTables(qboolean hasRedTable, qboolean hasBlueTable)
{
	cg_spectatorItemsHudRedTable = hasRedTable;
	cg_spectatorItemsHudBlueTable = hasBlueTable;
}

static int CG_SpectatorItems_Now(void)
{
	if (cg.snap)
	{
		return cg.snap->serverTime;
	}
	return cg.time;
}

static qboolean CG_SpectatorItems_ValidTrackId(int trackId)
{
	if (trackId < 0 || trackId >= CG_ITEM_NOTIFY_MAX_ENTRIES)
	{
		return qfalse;
	}
	return qtrue;
}

static cg_spectatorItemEntry_t* CG_SpectatorItems_Entry(int entryIndex)
{
	if (!CG_SpectatorItems_ValidTrackId(entryIndex))
	{
		return NULL;
	}
	return &cg_spectatorItems[entryIndex];
}

static cg_spectatorItemEntry_t* CG_SpectatorItems_EntryByTrackId(int trackId)
{
	if (!CG_SpectatorItems_ValidTrackId(trackId))
	{
		return NULL;
	}
	if (!cg_spectatorItems[trackId].active)
	{
		return NULL;
	}
	return &cg_spectatorItems[trackId];
}

static int CG_SpectatorItems_ResolveItemNum(int group);

static int CG_SpectatorItems_PowerupSortOrder(int pwTag)
{
	switch (pwTag)
	{
		case PW_QUAD:
			return 0;
		case PW_BATTLESUIT:
			return 1;
		case PW_REGEN:
			return 2;
		case PW_INVIS:
			return 3;
		case PW_HASTE:
			return 4;
		case PW_FLIGHT:
			return 5;
		default:
			return 100 + pwTag;
	}
}

static int CG_SpectatorItems_GroupTier(int group)
{
	switch (group)
	{
		case CG_ITEM_NOTIFY_GROUP_MH:
			return 0;
		case CG_ITEM_NOTIFY_GROUP_RA:
			return 1;
		case CG_ITEM_NOTIFY_GROUP_YA:
			return 2;
		default:
			if (group >= CG_ITEM_NOTIFY_GROUP_POWERUP_BASE)
			{
				return 100 + CG_SpectatorItems_PowerupSortOrder(group - CG_ITEM_NOTIFY_GROUP_POWERUP_BASE);
			}
			return 500 + group;
	}
}

static qboolean CG_SpectatorItems_PuiTypeToGroup(int type, int* outGroup)
{
	if (!outGroup)
	{
		return qfalse;
	}

	switch (type)
	{
		case PUI_MH:
			*outGroup = CG_ITEM_NOTIFY_GROUP_MH;
			return qtrue;
		case PUI_RA:
			*outGroup = CG_ITEM_NOTIFY_GROUP_RA;
			return qtrue;
		case PUI_YA:
			*outGroup = CG_ITEM_NOTIFY_GROUP_YA;
			return qtrue;
		case PUI_QD:
			*outGroup = CG_ITEM_NOTIFY_GROUP_POWERUP_BASE + PW_QUAD;
			return qtrue;
		case PUI_BS:
			*outGroup = CG_ITEM_NOTIFY_GROUP_POWERUP_BASE + PW_BATTLESUIT;
			return qtrue;
		case PUI_REG:
			*outGroup = CG_ITEM_NOTIFY_GROUP_POWERUP_BASE + PW_REGEN;
			return qtrue;
		case PUI_INV:
			*outGroup = CG_ITEM_NOTIFY_GROUP_POWERUP_BASE + PW_INVIS;
			return qtrue;
		case PUI_HST:
			*outGroup = CG_ITEM_NOTIFY_GROUP_POWERUP_BASE + PW_HASTE;
			return qtrue;
		case PUI_FLG:
			*outGroup = CG_ITEM_NOTIFY_GROUP_POWERUP_BASE + PW_FLIGHT;
			return qtrue;
		default:
			return qfalse;
	}
}

static qboolean CG_SpectatorItems_ApplyTrack(int trackId, int group, int side, int csOrder,
        qboolean havePadCoords, int padIx, int padIy, int padIz)
{
	cg_spectatorItemEntry_t* e;

	if (!CG_SpectatorItems_ValidTrackId(trackId) || !(cgs.itemCatalogMode & 2))
	{
		return qfalse;
	}
	if (side < 0 || side > 2)
	{
		return qfalse;
	}
	if (cg_spectatorItems[trackId].active)
	{
		return qfalse;
	}

	e = &cg_spectatorItems[trackId];
	memset(e, 0, sizeof(*e));
	e->active = qtrue;
	e->itemId = trackId;
	e->group = group;
	e->trackSide = side;
	e->nextSpawnTime = 0;
	e->csOrder = csOrder;
	e->havePadCoords = havePadCoords;
	e->padIx = padIx;
	e->padIy = padIy;
	e->padIz = padIz;
	return qtrue;
}

typedef struct
{
	int itemId;
	int nextSpawnTime;
} cg_spectatorItemSavedSpawn_t;

static void CG_SpectatorItems_RestoreSavedSpawn(const cg_spectatorItemSavedSpawn_t* saved)
{
	cg_spectatorItemEntry_t* e;

	if (!saved || saved->itemId < 0)
	{
		return;
	}
	if (saved->nextSpawnTime <= 0)
	{
		return;
	}

	e = CG_SpectatorItems_EntryByTrackId(saved->itemId);
	if (!e)
	{
		return;
	}

	e->nextSpawnTime = saved->nextSpawnTime;
}

static void CG_SpectatorItems_Clear(void)
{
	memset(cg_spectatorItems, 0, sizeof(cg_spectatorItems));
	cg_spectatorItemSpotCount = 0;
}

static int CG_SpectatorItems_PuiTypeFromAbbrev(const char* abbrev)
{
	const char* classname;
	gitem_t*    item;
	int         i;

	if (!abbrev || !abbrev[0])
	{
		return -1;
	}
	if (!CG_ShadowItems_ClassnameFromAbbrev(abbrev, &classname))
	{
		return -1;
	}
	for (i = 1; i < bg_numItems; i++)
	{
		item = &bg_itemlist[i];
		if (!Q_stricmp(item->classname, classname))
		{
			return BG_PuiTypeFromItem(item);
		}
	}
	return -1;
}

static void CG_SpectatorItems_BuildCatalogPayload(char* buf, int bufSize)
{
	const char* chunk;
	int         i;

	buf[0] = '\0';
#if 0
	if (cgs.gamename_unfreeze)
	{
		return;
	}
#endif
	for (i = 0; i < CS_OSP2BE_ITEM_CATALOG_COUNT; i++)
	{
		chunk = CG_ConfigString(CS_OSP2BE_ITEM_CATALOG0 + i);
		if (!chunk[0])
		{
			continue;
		}
		if (buf[0] && (int)strlen(buf) + (int)strlen(chunk) >= bufSize - 1)
		{
			CG_Printf(S_COLOR_RED "ERROR: item catalog client payload overflow\n");
			break;
		}
		Q_strcat(buf, bufSize, chunk);
	}
}

/*
 * One wire segment == one physical spot. Every PUI-mappable abbrev gets its
 * own tracked entry, numbered left-to-right (matching the server trackId
 * contract). Items joined by '/' share the spot (drawn as overlays); the
 * server already tells us the side, so no flag-distance guessing here.
 */
static void CG_SpectatorItems_ParseCatalogSegment(char* segment, int* csOrder, int* nextTrackId)
{
	char*       semi;
	char*       tok;
	char*       slash;
	int         ix;
	int         iy;
	int         iz;
	int         side;
	int         type;
	int         group;
	qboolean    hasCoords;
	cg_spectatorItemSpot_t* spot;

	if (!segment || !segment[0] || !csOrder || !nextTrackId)
	{
		return;
	}

	side = 0;
	ix = iy = iz = 0;
	hasCoords = qfalse;
	semi = strchr(segment, ';');
	if (semi)
	{
		*semi = '\0';
		semi++;
		if (Q_sscanf(semi, "%d,%d,%d,%d", &ix, &iy, &iz, &side) == 4)
		{
			if (side < 0 || side > 2)
			{
				return;
			}
			hasCoords = qtrue;
		}
		else if (Q_sscanf(semi, "%d,%d,%d", &ix, &iy, &iz) == 3)
		{
			/* server only omits side outside CTF, where every spot is neutral */
			side = 0;
			hasCoords = qtrue;
		}
		else if (Q_sscanf(semi, "%d", &side) != 1 || side < 0 || side > 2)
		{
			return;
		}
	}

	if (!segment[0] || cg_spectatorItemSpotCount >= CG_ITEM_SPAWNS_SPOT_MAX)
	{
		return;
	}

	spot = NULL;
	tok = segment;
	while (tok && tok[0])
	{
		slash = strchr(tok, '/');
		if (slash)
		{
			*slash = '\0';
		}

		type = CG_SpectatorItems_PuiTypeFromAbbrev(tok);
		if (type >= 0 && type < PUI_TYPE_COUNT
		        && CG_SpectatorItems_PuiTypeToGroup(type, &group)
		        && CG_SpectatorItems_ApplyTrack(*nextTrackId, group, side, *csOrder,
		                                        hasCoords, ix, iy, iz))
		{
			if (!spot)
			{
				spot = &cg_spectatorItemSpots[cg_spectatorItemSpotCount];
				spot->side = side;
				spot->csOrder = *csOrder;
				spot->memberCount = 0;
			}
			if (spot->memberCount < CG_ITEM_SPAWNS_SPOT_MEMBERS_MAX)
			{
				spot->members[spot->memberCount++] = *nextTrackId;
			}
			(*nextTrackId)++;
			(*csOrder)++;
		}

		if (slash)
		{
			tok = slash + 1;
		}
		else
		{
			tok = NULL;
		}
	}

	if (spot)
	{
		cg_spectatorItemSpotCount++;
	}
}

static void CG_SpectatorItems_ParseConfigString(const char* str)
{
	static char buf[MAX_STRING_CHARS * CS_OSP2BE_ITEM_CATALOG_COUNT];
	char* parse;
	char* segment;
	cg_spectatorItemSavedSpawn_t saved[CG_ITEM_NOTIFY_MAX_ENTRIES];
	int savedCount;
	int csOrder;
	int nextTrackId;
	int i;

	savedCount = 0;
	for (i = 0; i < CG_ITEM_NOTIFY_MAX_ENTRIES; i++)
	{
		if (cg_spectatorItems[i].active && cg_spectatorItems[i].itemId >= 0
		        && cg_spectatorItems[i].nextSpawnTime > 0)
		{
			if (savedCount < CG_ITEM_NOTIFY_MAX_ENTRIES)
			{
				saved[savedCount].itemId = cg_spectatorItems[i].itemId;
				saved[savedCount].nextSpawnTime = cg_spectatorItems[i].nextSpawnTime;
				savedCount++;
			}
		}
	}

	CG_SpectatorItems_Clear();

	if (!str || !str[0])
	{
		cgs.itemCatalogMode = 0;
		return;
	}

	Q_strncpyz(buf, str, sizeof(buf));
	parse = buf;
	cgs.itemCatalogMode = BG_ItemCatalogParseMode(parse[0]);
	if (cgs.itemCatalogMode < 0)
	{
		cgs.itemCatalogMode = 1;
	}
	else
	{
		parse++;
	}

	csOrder = 0;
	nextTrackId = 0;
	while (*parse)
	{
		segment = parse;
		while (*parse && *parse != '|')
		{
			parse++;
		}
		if (*parse == '|')
		{
			*parse = '\0';
			parse++;
		}

		if (segment[0] && Q_strncmp(segment, "sp;", 3) && Q_strncmp(segment, "tde;", 4))
		{
			CG_SpectatorItems_ParseCatalogSegment(segment, &csOrder, &nextTrackId);
		}
	}

	for (i = 0; i < savedCount; i++)
	{
		CG_SpectatorItems_RestoreSavedSpawn(&saved[i]);
	}
}

void CG_SpectatorItems_OnConfigStringChanged(void)
{
	static char payload[MAX_STRING_CHARS * CS_OSP2BE_ITEM_CATALOG_COUNT];

#if 0
	if (cgs.gamename_unfreeze)
	{
		return;
	}
#endif

	CG_SHUDRefreshSpectatorItemSpawnsTeamTables();
	CG_SpectatorItems_BuildCatalogPayload(payload, sizeof(payload));
	CG_SpectatorItems_ParseConfigString(payload);
}

qboolean CG_SpectatorItems_GetTrackPadCoords(int trackId, int* ix, int* iy, int* iz)
{
	cg_spectatorItemEntry_t* e;

	if (!ix || !iy || !iz || !CG_SpectatorItems_ValidTrackId(trackId))
	{
		return qfalse;
	}
	e = CG_SpectatorItems_EntryByTrackId(trackId);
	if (!e || !e->havePadCoords)
	{
		return qfalse;
	}
	*ix = e->padIx;
	*iy = e->padIy;
	*iz = e->padIz;
	return qtrue;
}

void CG_SpectatorItems_OnPickedUpItem(int pickerNum, int pickItemId, int nextSpawnTime,
                                      int nextItemId)
{
	cg_spectatorItemEntry_t* pick;
	cg_spectatorItemEntry_t* next;
	qboolean haveNext;

	if (pickItemId < 0 || pickItemId >= CG_ITEM_NOTIFY_MAX_ENTRIES)
	{
		return;
	}

	haveNext = (nextItemId >= 0 && nextItemId < CG_ITEM_NOTIFY_MAX_ENTRIES);
	pick = CG_SpectatorItems_EntryByTrackId(pickItemId);

	if (pick && pick->active && pickerNum >= 0 && pickerNum < MAX_CLIENTS)
	{
		CG_SHUDEventSpectatorItemPickup(pickerNum, pick->group);
	}

	next = NULL;
	if (haveNext)
	{
		next = CG_SpectatorItems_EntryByTrackId(nextItemId);
	}

	if (next)
	{
		int     modelindex;

		if (pick)
		{
			pick->nextSpawnTime = 0;
		}
		next->nextSpawnTime = nextSpawnTime;

		modelindex = CG_SpectatorItems_ResolveItemNum(next->group);
		if (modelindex > 0)
		{
			CG_ShadowItems_OnServerNextItem(modelindex, nextItemId);
		}
	}
	else if (pick)
	{
		pick->nextSpawnTime = nextSpawnTime;
	}
}

int CG_SpectatorItems_GetGroup(int entryIndex)
{
	cg_spectatorItemEntry_t* e;

	e = CG_SpectatorItems_Entry(entryIndex);
	if (!e || !e->active)
	{
		return -1;
	}
	return e->group;
}

int CG_SpectatorItems_GetNextSpawnTime(int entryIndex)
{
	cg_spectatorItemEntry_t* e;

	e = CG_SpectatorItems_Entry(entryIndex);
	if (!e || !e->active)
	{
		return 0;
	}
	return e->nextSpawnTime;
}

int CG_SpectatorItems_RemainingMs(int entryIndex)
{
	int spawnTime;

	spawnTime = CG_SpectatorItems_GetNextSpawnTime(entryIndex);
	if (spawnTime <= 0)
	{
		return 0;
	}
	return spawnTime - CG_SpectatorItems_Now();
}

static int CG_SpectatorItems_ResolveItemNum(int group)
{
	static int cache[CG_ITEM_NOTIFY_MAX_GROUPS];
	static qboolean cacheInit;
	int itemNum;
	gitem_t* gi;
	gitem_t* it;

	if (!cacheInit)
	{
		for (itemNum = 0; itemNum < CG_ITEM_NOTIFY_MAX_GROUPS; itemNum++)
		{
			cache[itemNum] = -1;
		}
		cacheInit = qtrue;
	}

	if (group >= 0 && group < CG_ITEM_NOTIFY_MAX_GROUPS && cache[group] != -1)
	{
		return cache[group];
	}

	gi = NULL;
	switch (group)
	{
		case CG_ITEM_NOTIFY_GROUP_MH:
			for (it = bg_itemlist + 1; it->classname; it++)
			{
				if (!Q_stricmp(it->classname, "item_health_mega"))
				{
					gi = it;
					break;
				}
			}
			break;
		case CG_ITEM_NOTIFY_GROUP_RA:
			for (it = bg_itemlist + 1; it->classname; it++)
			{
				if (!Q_stricmp(it->classname, "item_armor_body"))
				{
					gi = it;
					break;
				}
			}
			break;
		case CG_ITEM_NOTIFY_GROUP_YA:
			for (it = bg_itemlist + 1; it->classname; it++)
			{
				if (!Q_stricmp(it->classname, "item_armor_combat"))
				{
					gi = it;
					break;
				}
			}
			break;
		default:
			if (group >= CG_ITEM_NOTIFY_GROUP_POWERUP_BASE)
			{
				gi = BG_FindItemForPowerup(group - CG_ITEM_NOTIFY_GROUP_POWERUP_BASE);
			}
			break;
	}

	itemNum = gi ? ITEM_INDEX(gi) : 0;
	if (group >= 0 && group < CG_ITEM_NOTIFY_MAX_GROUPS)
	{
		cache[group] = itemNum;
	}
	return itemNum;
}

qhandle_t CG_SpectatorItems_IconShader(int group)
{
	int itemNum;

	itemNum = CG_SpectatorItems_ResolveItemNum(group);
	if (itemNum < 1 || itemNum >= bg_numItems)
	{
		return 0;
	}

	CG_RegisterItemVisuals(itemNum);
	if (!cg_items[itemNum].icon)
	{
		return 0;
	}

	return cg_items[itemNum].icon;
}

/*
 * A spot carries a single side (all its members come from one wire segment),
 * so the HUD side filter applies per spot. -1 is the generic element and
 * routes neutral/leftover sides depending on which team tables exist.
 */
static qboolean CG_SpectatorItems_SpotPassesSideFilter(const cg_spectatorItemSpot_t* spot, int trackSideFilter)
{
	if (!spot || spot->memberCount <= 0)
	{
		return qfalse;
	}

	if (trackSideFilter >= 0)
	{
		return (spot->side == trackSideFilter);
	}

	if (cg_spectatorItemsHudRedTable && cg_spectatorItemsHudBlueTable)
	{
		return (spot->side == 0);
	}
	if (cg_spectatorItemsHudRedTable)
	{
		return (spot->side != 1);
	}
	if (cg_spectatorItemsHudBlueTable)
	{
		return (spot->side != 2);
	}

	return qtrue;
}

static int CG_SpectatorItems_SpotDrawRefCompare(const void* a, const void* b)
{
	const cg_spectatorItemSpotDrawRef_t* refA;
	const cg_spectatorItemSpotDrawRef_t* refB;

	refA = (const cg_spectatorItemSpotDrawRef_t*)a;
	refB = (const cg_spectatorItemSpotDrawRef_t*)b;
	return refA->sortKey - refB->sortKey;
}

static void CG_SpectatorItems_SpotAddOverlay(int* groups, int* overlayOrders, int* count, int group,
        int csOrder)
{
	int i;
	int j;
	int insertAt;

	if (group < 0 || *count >= CG_ITEM_SPAWNS_OVERLAY_MAX)
	{
		return;
	}

	for (i = 0; i < *count; i++)
	{
		if (groups[i] == group)
		{
			return;
		}
	}

	insertAt = *count;
	for (j = 0; j < *count; j++)
	{
		if (csOrder < overlayOrders[j])
		{
			insertAt = j;
			break;
		}
	}

	for (j = *count; j > insertAt; j--)
	{
		groups[j] = groups[j - 1];
		overlayOrders[j] = overlayOrders[j - 1];
	}
	groups[insertAt] = group;
	overlayOrders[insertAt] = csOrder;
	(*count)++;
}

static int CG_SpectatorItems_SpotBestGroupTier(const int* members, int memberCount)
{
	int bestTier;
	int m;
	cg_spectatorItemEntry_t* member;

	bestTier = 999999;
	for (m = 0; m < memberCount; m++)
	{
		member = CG_SpectatorItems_Entry(members[m]);
		if (!member)
		{
			continue;
		}
		if (CG_SpectatorItems_GroupTier(member->group) < bestTier)
		{
			bestTier = CG_SpectatorItems_GroupTier(member->group);
		}
	}
	return bestTier;
}

static int CG_SpectatorItems_SpotPickPrimaryTrack(const int* members, int memberCount)
{
	int primaryTrack;
	int countdownTrack;
	int liveTrack;
	int bestTier;
	int bestRemaining;
	int bestSpawnTime;
	int m;
	int ms;
	int tier;
	cg_spectatorItemEntry_t* member;

	primaryTrack = members[0];
	countdownTrack = -1;
	liveTrack = -1;
	bestTier = 999999;
	bestRemaining = 0;
	bestSpawnTime = 0;

	for (m = 0; m < memberCount; m++)
	{
		member = CG_SpectatorItems_Entry(members[m]);
		if (!member)
		{
			continue;
		}

		ms = CG_SpectatorItems_RemainingMs(members[m]);
		if (ms > 0)
		{
			if (countdownTrack < 0 || ms < bestRemaining)
			{
				countdownTrack = members[m];
				bestRemaining = ms;
			}
		}
		else if (member->nextSpawnTime > 0)
		{
			if (liveTrack < 0 || member->nextSpawnTime > bestSpawnTime)
			{
				liveTrack = members[m];
				bestSpawnTime = member->nextSpawnTime;
			}
		}

		tier = CG_SpectatorItems_GroupTier(member->group);
		if (tier < bestTier)
		{
			bestTier = tier;
			primaryTrack = members[m];
		}
	}

	if (countdownTrack >= 0)
	{
		return countdownTrack;
	}
	if (liveTrack >= 0)
	{
		return liveTrack;
	}
	return primaryTrack;
}

int CG_SpectatorItems_BuildSpotDrawList(int trackSideFilter, cg_spectatorItemSpotDrawRef_t* out, int maxOut)
{
	cg_spectatorItemSpot_t* spot;
	cg_spectatorItemEntry_t* member;
	cg_spectatorItemSpotDrawRef_t* ref;
	int s;
	int m;
	int ms;
	int count;
	int primaryTrack;
	int bestRemaining;
	int overlayOrders[CG_ITEM_SPAWNS_OVERLAY_MAX];

	if (!out || maxOut <= 0 || !(cgs.itemCatalogMode & 2))
	{
		return 0;
	}

	count = 0;
	for (s = 0; s < cg_spectatorItemSpotCount && count < maxOut; s++)
	{
		spot = &cg_spectatorItemSpots[s];
		if (!CG_SpectatorItems_SpotPassesSideFilter(spot, trackSideFilter))
		{
			continue;
		}

		ref = &out[count];
		memset(ref, 0, sizeof(*ref));
		ref->timerTrackId = -1;
		bestRemaining = 0;

		for (m = 0; m < spot->memberCount; m++)
		{
			ms = CG_SpectatorItems_RemainingMs(spot->members[m]);
			if (ms <= 0)
			{
				continue;
			}
			if (ref->timerTrackId < 0 || ms < bestRemaining)
			{
				bestRemaining = ms;
				ref->timerTrackId = spot->members[m];
			}
		}

		primaryTrack = CG_SpectatorItems_SpotPickPrimaryTrack(spot->members, spot->memberCount);
		if (ref->timerTrackId >= 0 && CG_SpectatorItems_RemainingMs(ref->timerTrackId) > 0)
		{
			primaryTrack = ref->timerTrackId;
		}

		member = CG_SpectatorItems_Entry(primaryTrack);
		if (!member)
		{
			continue;
		}

		ref->primaryGroup = member->group;
		ref->sortKey = CG_SpectatorItems_SpotBestGroupTier(spot->members, spot->memberCount) * 4096
		               + spot->side * 1024 + (spot->csOrder & 1023);

		for (m = 0; m < spot->memberCount; m++)
		{
			member = CG_SpectatorItems_Entry(spot->members[m]);
			if (!member)
			{
				continue;
			}
			if (member->group == ref->primaryGroup)
			{
				continue;
			}
			CG_SpectatorItems_SpotAddOverlay(ref->overlayGroups, overlayOrders, &ref->overlayCount,
			                                 member->group, member->csOrder);
		}

		count++;
	}

	if (count > 1)
	{
		qsort(out, count, sizeof(out[0]), CG_SpectatorItems_SpotDrawRefCompare);
	}

	return count;
}

qboolean CG_SpectatorItems_ServerCommand(void)
{
	const char* cmd;
	int argc;

	cmd = CG_Argv(0);
	if (!cmd || !cmd[0])
	{
		return qfalse;
	}

	if (cmd[0] == 'p' && cmd[1] == 'u' && cmd[2] == 'i' && cmd[3] == '\0')
	{
		int nextSpawnTime;
		int pickerNum;
		int pickId;
		int nextId;
		int packed;

		argc = trap_Argc();
		if (argc >= 3)
		{
			packed = atoi(CG_Argv(1));
			nextSpawnTime = atoi(CG_Argv(2));
			BG_PuiUnpackCmd(packed, &pickerNum, &pickId, &nextId);
			if (pickId >= 0 && pickId < CG_ITEM_NOTIFY_MAX_ENTRIES)
			{
				CG_SpectatorItems_OnPickedUpItem(pickerNum, pickId, nextSpawnTime, nextId);
			}
		}
		return qtrue;
	}

	return qfalse;
}
