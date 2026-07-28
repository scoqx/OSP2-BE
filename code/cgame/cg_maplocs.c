/*
===========================================================================
cg_maplocs.c

Map zones for tinfo -> world coords (cg_friendsWallhack bit 4).
Data: QLM1 section in mapdata Q3LB (mapdata/mapdata.q3lb or mapdata/<map>.q3lb).

Copyright (c) 2026 diwoc
===========================================================================
*/

#include "cg_local.h"
#include "../qcommon/qcommon.h"

#define CG_MAPLOC_NAME_LEN      64

typedef struct
{
	int     id;
	int     color;
	vec3_t  origin;
	char    name[CG_MAPLOC_NAME_LEN];
} cg_mapLoc_t;

static cg_mapLoc_t cg_mapLocs[MAX_LOCATIONS];
static int cg_mapLocCount;
static qboolean cg_mapLocOriginValid[MAX_LOCATIONS];
static vec3_t cg_mapLocOriginById[MAX_LOCATIONS];
static int cg_mapLocServerOriginCount;
static qboolean cg_mapLocServerNamesCached;
static qboolean cg_mapLocServerNamesPresent;

static void CG_MapLocs_InvalidateServerLocationNamesCache(void)
{
	cg_mapLocServerNamesCached = qfalse;
}

const char *CG_LocationName(int locationId)
{
	if (locationId < 0 || locationId >= MAX_LOCATIONS)
	{
		return "";
	}
	return CG_ConfigString(CS_LOCATIONS + locationId);
}

static void CG_MapLocs_RefreshServerLocationNamesCache(void)
{
	int i;
	const char* p;

	cg_mapLocServerNamesPresent = qfalse;
	for (i = 1; i < MAX_LOCATIONS; i++)
	{
		p = CG_ConfigString(CS_LOCATIONS + i);
		if (p && p[0])
		{
			cg_mapLocServerNamesPresent = qtrue;
			break;
		}
	}
	cg_mapLocServerNamesCached = qtrue;
}

void CG_MapLocs_NotifyLocationConfigStringChanged(void)
{
	CG_MapLocs_InvalidateServerLocationNamesCache();
}

static void CG_MapLocs_Clear(void)
{
	cg_mapLocCount = 0;
	cg_mapLocServerOriginCount = 0;
	memset(cg_mapLocOriginValid, 0, sizeof(cg_mapLocOriginValid));
	CG_MapLocs_InvalidateServerLocationNamesCache();
}

qboolean CG_MapLocs_LoadQLM1Binary(const byte* raw, int rawLen)
{
	int pos;
	int mapLen;
	int count;
	int i;
	int id;
	float ox;
	float oy;
	float oz;
	int color;
	int nameLen;
	int copyLen;

	if (rawLen < 6 || raw[0] != 'Q' || raw[1] != 'L' || raw[2] != 'M' || raw[3] != '1')
	{
		return qfalse;
	}

	pos = 4;
	mapLen = raw[pos++];
	if (mapLen <= 0 || pos + mapLen > rawLen)
	{
		return qfalse;
	}
	pos += mapLen;

	if (pos >= rawLen)
	{
		return qfalse;
	}
	count = raw[pos++];
	if (count <= 0)
	{
		return qfalse;
	}

	cg_mapLocCount = 0;
	memset(cg_mapLocOriginValid, 0, sizeof(cg_mapLocOriginValid));

	for (i = 0; i < count; i++)
	{
		if (pos + 15 > rawLen)
		{
			break;
		}

		id = raw[pos++];
		{
			int bits;

			memcpy(&bits, raw + pos, 4);
			ox = *(float*)&bits;
			pos += 4;
			memcpy(&bits, raw + pos, 4);
			oy = *(float*)&bits;
			pos += 4;
			memcpy(&bits, raw + pos, 4);
			oz = *(float*)&bits;
			pos += 4;
		}
		color = raw[pos++];
		nameLen = raw[pos++];

		if (pos + nameLen > rawLen)
		{
			break;
		}

		if (id <= 0 || id >= MAX_LOCATIONS || cg_mapLocCount >= MAX_LOCATIONS)
		{
			pos += nameLen;
			continue;
		}

		cg_mapLocs[cg_mapLocCount].id = id;
		cg_mapLocs[cg_mapLocCount].color = color;
		cg_mapLocs[cg_mapLocCount].origin[0] = ox;
		cg_mapLocs[cg_mapLocCount].origin[1] = oy;
		cg_mapLocs[cg_mapLocCount].origin[2] = oz;
		copyLen = nameLen;
		if (copyLen >= CG_MAPLOC_NAME_LEN)
		{
			copyLen = CG_MAPLOC_NAME_LEN - 1;
		}
		memcpy(cg_mapLocs[cg_mapLocCount].name, raw + pos, copyLen);
		cg_mapLocs[cg_mapLocCount].name[copyLen] = '\0';
		pos += nameLen;
		cg_mapLocOriginById[id][0] = ox;
		cg_mapLocOriginById[id][1] = oy;
		cg_mapLocOriginById[id][2] = oz;
		cg_mapLocOriginValid[id] = qtrue;
		cg_mapLocCount++;
	}

	return (cg_mapLocCount > 0) ? qtrue : qfalse;
}

qboolean CG_MapLocs_Load(void)
{
	const byte* qlm;
	int         qlmLen;

	CG_MapLocs_Clear();
	if (!CG_MapBundle_Section("QLM1", &qlm, &qlmLen))
	{
		return qfalse;
	}
	return CG_MapLocs_LoadQLM1Binary(qlm, qlmLen);
}

qboolean CG_MapLocs_GetById(int id, vec3_t origin, char* name, int nameSize)
{
	int i;

	if (id <= 0 || id >= MAX_LOCATIONS)
	{
		return qfalse;
	}

	if (origin && cg_mapLocOriginValid[id])
	{
		VectorCopy(cg_mapLocOriginById[id], origin);
		if (!name || nameSize <= 0)
		{
			return qtrue;
		}
	}

	for (i = 0; i < cg_mapLocCount; i++)
	{
		if (cg_mapLocs[i].id == id)
		{
			if (origin)
			{
				VectorCopy(cg_mapLocs[i].origin, origin);
			}
			if (name && nameSize > 0)
			{
				Q_strncpyz(name, cg_mapLocs[i].name, nameSize);
			}
			return qtrue;
		}
	}

	return qfalse;
}

qboolean CG_MapLocs_Count(void)
{
	return (cg_mapLocCount > 0 || cg_mapLocServerOriginCount > 0) ? qtrue : qfalse;
}

qboolean CG_MapLocs_HasLocalZones(void)
{
	return (cg_mapLocCount > 0) ? qtrue : qfalse;
}

/*
 * locs <id> <x10> <y10> <z10> ... — server origins aligned with CS_LOCATIONS ids.
 * Ignored when local QLM1 zones are already loaded.
 */
void CG_MapLocs_ApplyServerOrigins(void)
{
	int argc;
	int i;
	int id;
	int ix, iy, iz;

	if (cg_mapLocCount > 0)
	{
		return;
	}

	argc = trap_Argc();
	/* cmd + groups of 4 */
	for (i = 1; i + 3 < argc; i += 4)
	{
		id = atoi(CG_Argv(i));
		ix = atoi(CG_Argv(i + 1));
		iy = atoi(CG_Argv(i + 2));
		iz = atoi(CG_Argv(i + 3));
		if (id <= 0 || id >= MAX_LOCATIONS)
		{
			continue;
		}
		if (!cg_mapLocOriginValid[id])
		{
			cg_mapLocServerOriginCount++;
		}
		cg_mapLocOriginById[id][0] = ix / 10.0f;
		cg_mapLocOriginById[id][1] = iy / 10.0f;
		cg_mapLocOriginById[id][2] = iz / 10.0f;
		cg_mapLocOriginValid[id] = qtrue;
	}
}

qboolean CG_MapLocs_ServerHasLocationNames(void)
{
	if (!cg_mapLocServerNamesCached)
	{
		CG_MapLocs_RefreshServerLocationNamesCache();
	}
	return cg_mapLocServerNamesPresent;
}

int CG_MapLocs_NumZones(void)
{
	return cg_mapLocCount;
}

const char* CG_MapLocs_DataSource(void)
{
	if (cg_mapLocCount > 0)
	{
		return CG_MapBundle_DataSource();
	}
	if (cg_mapLocServerOriginCount > 0)
	{
		return "server";
	}
	return NULL;
}

void CG_MapLocs_PrintList(int maxList)
{
	int i;
	int n;

	if (!cg_mapLocCount)
	{
		CG_Printf("  ^1no location zones loaded^7\n");
		return;
	}

	n = cg_mapLocCount;
	if (maxList > 0 && maxList < n)
	{
		n = maxList;
	}

	CG_Printf("  ^3locations^7 (%i):\n", cg_mapLocCount);
	for (i = 0; i < n; i++)
	{
		const char* csName;

		csName = CG_LocationName(cg_mapLocs[i].id);
		CG_Printf("  id ^3%2d^7  ^5%.0f %.0f %.0f^7  %s%s^7\n",
		          cg_mapLocs[i].id,
		          cg_mapLocs[i].origin[0],
		          cg_mapLocs[i].origin[1],
		          cg_mapLocs[i].origin[2],
		          (csName && csName[0]) ? "^2[CS] " : "",
		          cg_mapLocs[i].name);
	}
	if (n < cg_mapLocCount)
	{
		CG_Printf("  ... (%i more, use testq3lb list locs <count>)\n", cg_mapLocCount - n);
	}
}
