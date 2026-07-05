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

#include "cg_local.h"
#include "superhud.h"

typedef struct
{
	qboolean active;
	int itemId;
	int group;
	int slotIdx;
	int nextSpawnTime;
	int trackSide;
	int csOrder;
	int hudPadGroup;
	int padOrder;
} cg_spectatorItemEntry_t;

static cg_spectatorItemEntry_t cg_spectatorItems[CG_ITEM_NOTIFY_MAX_ENTRIES];
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

static cg_spectatorItemEntry_t* CG_SpectatorItems_EntryByItemId(int itemId)
{
	int i;

	if (itemId < 0 || itemId >= PUI_ID_INVALID)
	{
		return NULL;
	}

	for (i = 0; i < CG_ITEM_NOTIFY_MAX_ENTRIES; i++)
	{
		if (cg_spectatorItems[i].active && cg_spectatorItems[i].itemId == itemId)
		{
			return &cg_spectatorItems[i];
		}
	}
	return NULL;
}

static int CG_SpectatorItems_AllocEntry(void)
{
	int i;

	for (i = 0; i < CG_ITEM_NOTIFY_MAX_ENTRIES; i++)
	{
		if (!cg_spectatorItems[i].active)
		{
			return i;
		}
	}
	return -1;
}

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

static qboolean CG_SpectatorItems_ApplyItemId(int itemId, int csOrder, int hudPadGroup, int padOrder)
{
	int type;
	int side;
	int slot;
	int group;
	int entryIndex;
	cg_spectatorItemEntry_t* e;

	if (itemId < 0 || itemId >= PUI_ID_INVALID)
	{
		return qfalse;
	}

	if (CG_SpectatorItems_EntryByItemId(itemId))
	{
		return qfalse;
	}

	type = BG_PuiIdType(itemId);
	side = BG_PuiIdSide(itemId);
	slot = BG_PuiIdSlot(itemId);

	if (type < 0 || type >= PUI_TYPE_COUNT || side < 0 || side > 2)
	{
		return qfalse;
	}

	if (!CG_SpectatorItems_PuiTypeToGroup(type, &group))
	{
		return qfalse;
	}

	entryIndex = CG_SpectatorItems_AllocEntry();
	if (entryIndex < 0)
	{
		return qfalse;
	}

	e = &cg_spectatorItems[entryIndex];
	memset(e, 0, sizeof(*e));
	e->active = qtrue;
	e->itemId = itemId;
	e->group = group;
	e->slotIdx = slot;
	e->trackSide = side;
	e->nextSpawnTime = 0;
	e->csOrder = csOrder;
	e->hudPadGroup = hudPadGroup;
	e->padOrder = padOrder;
	return qtrue;
}

static int CG_SpectatorItems_DisplayOrderKey(const cg_spectatorItemEntry_t* e)
{
	int tier;

	if (!e)
	{
		return 999999;
	}

	tier = CG_SpectatorItems_GroupTier(e->group);
	/* mh, ra, ya, powerups (metarena abbrevOrder / shadow abbrevs); then idx; n/r/b side */
	return tier * 4096 + e->slotIdx * 16 + e->trackSide;
}

static int CG_SpectatorItems_DrawRefCompare(const void* a, const void* b)
{
	const cg_spectatorItemDrawRef_t* refA;
	const cg_spectatorItemDrawRef_t* refB;
	cg_spectatorItemEntry_t* entryA;
	cg_spectatorItemEntry_t* entryB;
	int keyA;
	int keyB;

	refA = (const cg_spectatorItemDrawRef_t*)a;
	refB = (const cg_spectatorItemDrawRef_t*)b;
	entryA = CG_SpectatorItems_Entry(refA->trackId);
	entryB = CG_SpectatorItems_Entry(refB->trackId);
	if (!entryA || !entryB)
	{
		return 0;
	}
	keyA = CG_SpectatorItems_DisplayOrderKey(entryA);
	keyB = CG_SpectatorItems_DisplayOrderKey(entryB);
	if (keyA != keyB)
	{
		return keyA - keyB;
	}
	return refA->trackId - refB->trackId;
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

	e = CG_SpectatorItems_EntryByItemId(saved->itemId);
	if (!e)
	{
		return;
	}

	e->nextSpawnTime = saved->nextSpawnTime;
}

static void CG_SpectatorItems_Clear(void)
{
	memset(cg_spectatorItems, 0, sizeof(cg_spectatorItems));
}

static qboolean CG_SpectatorItems_ParseFlatItemId(const char* tok, int* csOrder)
{
	int itemId;

	itemId = atoi(tok);
	if (itemId < 0 || itemId >= PUI_ID_INVALID)
	{
		return qfalse;
	}
	if (CG_SpectatorItems_ApplyItemId(itemId, *csOrder, -1, 0))
	{
		(*csOrder)++;
		return qtrue;
	}
	return qfalse;
}

static qboolean CG_SpectatorItems_ParseV4Segment(char* segment, int* csOrder)
{
	char* tok;
	int side;
	int padIdx;
	int hudPadGroup;
	int padOrder;
	int itemId;

	if (!segment || !segment[0] || !csOrder)
	{
		return qfalse;
	}

	if (segment[0] == 'I' && segment[1] == ',')
	{
		return CG_SpectatorItems_ParseFlatItemId(segment + 2, csOrder);
	}

	if (segment[0] != 'S' || segment[1] != ',')
	{
		return qfalse;
	}

	tok = segment + 2;
	side = atoi(tok);
	tok = strchr(tok, ',');
	if (!tok)
	{
		return qfalse;
	}
	tok++;
	padIdx = atoi(tok);
	if (side < 0 || side > 2 || padIdx < 0 || padIdx > 15)
	{
		return qfalse;
	}

	hudPadGroup = (side << 4) | padIdx;
	padOrder = 0;

	for (;;)
	{
		tok = strchr(tok, ',');
		if (!tok)
		{
			break;
		}
		tok++;
		if (!tok[0])
		{
			break;
		}

		itemId = atoi(tok);
		if (itemId >= 0 && itemId < PUI_ID_INVALID)
		{
			if (CG_SpectatorItems_ApplyItemId(itemId, *csOrder, hudPadGroup, padOrder))
			{
				(*csOrder)++;
				padOrder++;
			}
		}
	}

	return (padOrder > 0);
}

static void CG_SpectatorItems_ParseConfigString(const char* str)
{
	char buf[MAX_STRING_CHARS];
	char* parse;
	char* segment;
	cg_spectatorItemSavedSpawn_t saved[CG_ITEM_NOTIFY_MAX_ENTRIES];
	int savedCount;
	int csOrder;
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
		return;
	}

	if (Q_stricmpn(str, "v4", 2) != 0 || (str[2] != '\0' && str[2] != ';'))
	{
		return;
	}

	Q_strncpyz(buf, str, sizeof(buf));
	parse = buf;
	parse += 2;
	if (*parse == ';')
	{
		parse++;
	}

	csOrder = 0;
	while (*parse)
	{
		segment = parse;
		while (*parse && *parse != ';')
		{
			parse++;
		}
		if (*parse == ';')
		{
			*parse = '\0';
			parse++;
		}

		if (segment[0])
		{
			CG_SpectatorItems_ParseV4Segment(segment, &csOrder);
		}
	}

	for (i = 0; i < savedCount; i++)
	{
		CG_SpectatorItems_RestoreSavedSpawn(&saved[i]);
	}
}

void CG_SpectatorItems_OnConfigStringChanged(void)
{
	CG_SHUDRefreshSpectatorItemSpawnsTeamTables();
	CG_SpectatorItems_ParseConfigString(CG_ConfigString(CS_OSP2BE_ITEM_TRACK));
}

static cg_spectatorItemEntry_t* CG_SpectatorItems_PadNextEntry(const cg_spectatorItemEntry_t* pick)
{
	cg_spectatorItemEntry_t* e;
	cg_spectatorItemEntry_t* successor;
	cg_spectatorItemEntry_t* wrap;
	int bestOrder;
	int minOrder;
	int i;

	if (!pick || pick->hudPadGroup < 0)
	{
		return NULL;
	}

	successor = NULL;
	wrap = NULL;
	bestOrder = 999999999;
	minOrder = 999999999;

	for (i = 0; i < CG_ITEM_NOTIFY_MAX_ENTRIES; i++)
	{
		e = &cg_spectatorItems[i];
		if (!e->active || e->hudPadGroup != pick->hudPadGroup || e == pick)
		{
			continue;
		}

		if (e->padOrder > pick->padOrder && e->padOrder < bestOrder)
		{
			bestOrder = e->padOrder;
			successor = e;
		}
		if (e->padOrder < minOrder)
		{
			minOrder = e->padOrder;
			wrap = e;
		}
	}

	if (successor)
	{
		return successor;
	}
	return wrap;
}

void CG_SpectatorItems_OnPickedUpItem(int pickerNum, int pickItemId, int nextSpawnTime,
                                      int nextItemId)
{
	cg_spectatorItemEntry_t* pick;
	cg_spectatorItemEntry_t* next;
	qboolean haveNext;

	if (pickItemId < 0 || pickItemId >= PUI_ID_INVALID)
	{
		return;
	}

	haveNext = (nextItemId >= 0 && nextItemId < PUI_ID_INVALID && nextItemId != pickItemId);
	pick = CG_SpectatorItems_EntryByItemId(pickItemId);

	if (pick && pick->active && pickerNum >= 0 && pickerNum < MAX_CLIENTS)
	{
		CG_SHUDEventSpectatorItemPickup(pickerNum, pick->group);
	}

	next = NULL;
	if (haveNext)
	{
		next = CG_SpectatorItems_EntryByItemId(nextItemId);
	}
	if (!next && pick && pick->hudPadGroup >= 0 && nextSpawnTime > 0)
	{
		next = CG_SpectatorItems_PadNextEntry(pick);
	}

	if (next)
	{
		if (pick)
		{
			pick->nextSpawnTime = 0;
		}
		next->nextSpawnTime = nextSpawnTime;
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
	gitem_t* gi;
	gitem_t* it;

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

	if (!gi)
	{
		return 0;
	}
	return ITEM_INDEX(gi);
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

static qboolean CG_SpectatorItems_EntryPassesSideFilter(const cg_spectatorItemEntry_t* e, int trackSideFilter)
{
	if (!e || !e->active)
	{
		return qfalse;
	}

	if (trackSideFilter >= 0)
	{
		return (e->trackSide == trackSideFilter);
	}

	if (cg_spectatorItemsHudRedTable && cg_spectatorItemsHudBlueTable)
	{
		return (e->trackSide == 0);
	}
	if (cg_spectatorItemsHudRedTable)
	{
		return (e->trackSide != 1);
	}
	if (cg_spectatorItemsHudBlueTable)
	{
		return (e->trackSide != 2);
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

static int CG_SpectatorItems_SpotPickPrimaryTrack(const int* members, int memberCount)
{
	int primaryTrack;
	int countdownTrack;
	int liveTrack;
	int bestPadOrder;
	int bestRemaining;
	int bestSpawnTime;
	int m;
	int ms;
	cg_spectatorItemEntry_t* member;

	primaryTrack = members[0];
	countdownTrack = -1;
	liveTrack = -1;
	bestPadOrder = 999999999;
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

		if (member->padOrder < bestPadOrder)
		{
			bestPadOrder = member->padOrder;
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
	typedef struct
	{
		int padGroup;
		int members[CG_ITEM_SPAWNS_SPOT_MEMBERS_MAX];
		int memberCount;
	} spotAccum_t;

	spotAccum_t spots[CG_ITEM_SPAWNS_SPOT_MAX];
	int spotCount;
	int trackId;
	int i;
	int s;
	int m;
	int count;
	cg_spectatorItemEntry_t* e;
	cg_spectatorItemEntry_t* member;
	spotAccum_t* spot;
	cg_spectatorItemSpotDrawRef_t* ref;
	int primaryTrack;
	int bestRemaining;
	int overlayOrders[CG_ITEM_SPAWNS_OVERLAY_MAX];

	if (!out || maxOut <= 0)
	{
		return 0;
	}

	memset(spots, 0, sizeof(spots));
	spotCount = 0;

	for (trackId = 0; trackId < CG_ITEM_NOTIFY_MAX_ENTRIES; trackId++)
	{
		e = &cg_spectatorItems[trackId];
		if (!CG_SpectatorItems_EntryPassesSideFilter(e, trackSideFilter))
		{
			continue;
		}

		if (e->hudPadGroup < 0)
		{
			if (spotCount >= CG_ITEM_SPAWNS_SPOT_MAX)
			{
				continue;
			}
			spot = &spots[spotCount];
			spot->padGroup = -1 - trackId;
			spot->memberCount = 1;
			spot->members[0] = trackId;
			spotCount++;
			continue;
		}

		spot = NULL;
		for (s = 0; s < spotCount; s++)
		{
			if (spots[s].padGroup == e->hudPadGroup)
			{
				spot = &spots[s];
				break;
			}
		}

		if (!spot)
		{
			if (spotCount >= CG_ITEM_SPAWNS_SPOT_MAX)
			{
				continue;
			}
			spot = &spots[spotCount];
			spot->padGroup = e->hudPadGroup;
			spotCount++;
		}

		if (spot->memberCount < CG_ITEM_SPAWNS_SPOT_MEMBERS_MAX)
		{
			spot->members[spot->memberCount] = trackId;
			spot->memberCount++;
		}
	}

	count = 0;
	for (s = 0; s < spotCount && count < maxOut; s++)
	{
		spot = &spots[s];
		if (spot->memberCount <= 0)
		{
			continue;
		}

		ref = &out[count];
		memset(ref, 0, sizeof(*ref));
		ref->timerTrackId = -1;
		bestRemaining = 0;

		for (m = 0; m < spot->memberCount; m++)
		{
			i = CG_SpectatorItems_RemainingMs(spot->members[m]);
			if (i <= 0)
			{
				continue;
			}
			if (ref->timerTrackId < 0 || i < bestRemaining)
			{
				bestRemaining = i;
				ref->timerTrackId = spot->members[m];
			}
		}

		primaryTrack = CG_SpectatorItems_SpotPickPrimaryTrack(spot->members, spot->memberCount);

		member = CG_SpectatorItems_Entry(primaryTrack);
		if (!member)
		{
			continue;
		}

		ref->primaryGroup = member->group;
		ref->sortKey = CG_SpectatorItems_DisplayOrderKey(member);

		if (spot->memberCount > 1 && spot->padGroup >= 0)
		{
			for (m = 0; m < spot->memberCount; m++)
			{
				member = CG_SpectatorItems_Entry(spot->members[m]);
				if (!member || member->group == ref->primaryGroup)
				{
					continue;
				}
				CG_SpectatorItems_SpotAddOverlay(ref->overlayGroups, overlayOrders, &ref->overlayCount,
				                               member->group, member->padOrder);
			}
		}

		count++;
	}

	if (count > 1)
	{
		qsort(out, count, sizeof(out[0]), CG_SpectatorItems_SpotDrawRefCompare);
	}

	return count;
}

int CG_SpectatorItems_BuildDrawList(int trackSideFilter, cg_spectatorItemDrawRef_t* out, int maxOut)
{
	int trackId;
	int count;
	cg_spectatorItemEntry_t* e;

	if (!out || maxOut <= 0)
	{
		return 0;
	}

	count = 0;
	for (trackId = 0; trackId < CG_ITEM_NOTIFY_MAX_ENTRIES; trackId++)
	{
		e = &cg_spectatorItems[trackId];
		if (!CG_SpectatorItems_EntryPassesSideFilter(e, trackSideFilter))
		{
			continue;
		}
		if (count >= maxOut)
		{
			break;
		}
		out[count].trackId = trackId;
		count++;
	}

	if (count > 1)
	{
		qsort(out, count, sizeof(out[0]), CG_SpectatorItems_DrawRefCompare);
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
		int packed;
		int nextSpawnTime;
		int pickerNum;
		int pickId;
		int nextId;

		argc = trap_Argc();
		if (argc >= 3)
		{
			packed = atoi(CG_Argv(1));
			nextSpawnTime = atoi(CG_Argv(2));
			BG_PuiUnpackCmd(packed, &pickerNum, &pickId, &nextId);

			if (pickId >= 0 && pickId < PUI_ID_INVALID)
			{
				CG_SpectatorItems_OnPickedUpItem(pickerNum, pickId, nextSpawnTime, nextId);
			}
		}
		return qtrue;
	}

	return qfalse;
}
